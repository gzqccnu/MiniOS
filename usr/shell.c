/*
 * Lrix
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 *
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 *
 * Project homepage: https://github.com/lrisguan/Lrix
 * Description: A scratch implemention of OS based on RISC-V
 */

// user-space shell for Lrix (runs as a regular process using syscalls)

#include "../kernel/include/log.h"
#include "user.h"

// ---- basic user helpers ----
// Current output target: -1 means stdout (fd=1), otherwise a specific fd (for pipe redirection)
static int shell_out_fd = -1;

static void uwrite_buf(const char *s, int len) {
  if (!s || len <= 0)
    return;
  int fd = (shell_out_fd >= 0) ? shell_out_fd : 1;
  sys_write(fd, s, (uint64_t)len);
}

static void uputs(const char *s) {
  if (!s)
    return;
  uwrite_buf(s, (int)strlen(s));
}

static void uputc(char c) { uwrite_buf(&c, 1); }

static void uprompt(const char *user, const char *host) {
  (void)user;
  (void)host;
  uputs(RED "Lrix" GREEN "$ " RESET);
}

static int readline(char *buf, int maxlen) {
  if (!buf || maxlen <= 1)
    return 0;
  int i = 0;
  while (i < maxlen - 1) {
    int ch = sys_getc();
    if (ch < 0)
      break;
    char c = (char)ch;
    if (c == '\r' || c == '\n') {
      uputc('\n');
      break;
    }
    // simple local editing: treat backspace
    if (c == 127 || c == '\b') {
      if (i > 0) {
        i--;
        uputs("\b \b");
      }
      continue;
    }
    buf[i++] = c;
    uputc(c);
  }
  buf[i] = '\0';
  return i;
}

// ---- command implementations ----
#define MAX_ARGS 8

static int parse_args(char *line, char *argv[], int max_args) {
  int argc = 0;
  char *p = line;
  while (*p && argc < max_args) {
    while (*p == ' ' || *p == '\t')
      p++;
    if (!*p)
      break;
    argv[argc++] = p;
    while (*p && *p != ' ' && *p != '\t')
      p++;
    if (*p == '\0')
      break;
    *p++ = '\0';
  }
  return argc;
}

// ---- simple pipeline support ----

int pipe_input_active = 0;
const char *pipe_input_name = "__pipe.tmp";

static char *trim_spaces(char *s) {
  while (*s == ' ' || *s == '\t')
    s++;
  if (*s == '\0')
    return s;
  char *end = s + strlen(s) - 1;
  while (end > s && (*end == ' ' || *end == '\t')) {
    *end = '\0';
    end--;
  }
  return s;
}

static void cmd_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    uputs(argv[i]);
    if (i + 1 < argc)
      uputc(' ');
  }
  uputc('\n');
}

static void cmd_ls(void) {
  struct dirent ents[FS_MAX_FILES];
  int n = sys_ls(ents, FS_MAX_FILES);
  if (n < 0) {
    uputs("ls: error\n");
    return;
  }
  for (int i = 0; i < n; i++) {
    if (ents[i].name[0]) {
      uputs(ents[i].name);
      uputc('\n');
    }
  }
}

static void cmd_cat(int argc, char *argv[]) {
  extern int pipe_input_active;
  extern const char *pipe_input_name;

  int fd;
  if (argc < 2) {
    if (!pipe_input_active) {
      uputs("cat: missing file name\n");
      return;
    }
    fd = sys_open(pipe_input_name, 0);
  } else {
    fd = sys_open(argv[1], 0);
  }
  if (fd < 0) {
    uputs("cat: cannot open file\n");
    return;
  }
  char buf[128];
  while (1) {
    long r = sys_read(fd, buf, sizeof(buf));
    if (r <= 0)
      break;
    uwrite_buf(buf, (int)r);
  }
  sys_close(fd);
}

static void cmd_help(void) {
  uputs("Builtin commands:\n");
  uputs("  ls        - list files in root\n");
  uputs("  cat FILE  - show file contents\n");
  uputs("  echo ...  - print arguments\n");
  uputs("  touch F   - create file if not exists\n");
  uputs("  rm F      - remove file\n");
  uputs("  mv A B    - move/rename file A to B\n");
  uputs("  pwd       - print current directory (always / in flat fs)\n");
  uputs("  mkdir D   - not supported (flat fs)\n");
  uputs("  rmdir D   - not supported (flat fs)\n");
  uputs("  write F S - write string S to file F\n");
  uputs("  read F    - read and print file F\n");
  uputs("  fork      - test fork() syscall\n");
  uputs("  bg        - create a simple background worker process\n");
  uputs("  kill PID  - kill process by pid\n");
  uputs("  ps        - list processes\n");
  uputs("  help      - show this message\n");
  uputs("  exit      - shutdown system\n");
  uputs("  halt      - shutdown whole system\n");
}

static void execute(int argc, char *argv[]) {
  if (argc == 0)
    return;
  if (strcmp(argv[0], "exit") == 0) {
    uputs("Shutting down system...\n");
    uputs("You can type \'Ctrl + Alt + X\' to exit qemu emulator.\n");
    sys_shutdown();
  } else if (strcmp(argv[0], "halt") == 0) {
    uputs("Shutting down system...\n");
    sys_shutdown();
  } else if (strcmp(argv[0], "echo") == 0) {
    cmd_echo(argc, argv);
  } else if (strcmp(argv[0], "ls") == 0) {
    cmd_ls();
  } else if (strcmp(argv[0], "cat") == 0) {
    cmd_cat(argc, argv);
  } else if (strcmp(argv[0], "read") == 0) {
    cmd_cat(argc, argv);
  } else if (strcmp(argv[0], "ps") == 0) {
    sys_ps();
  } else if (strcmp(argv[0], "touch") == 0) {
    if (argc < 2) {
      uputs("touch: missing file name\n");
    } else {
      // if exists, just open and close; if not, create
      int fd = sys_open(argv[1], 0);
      if (fd < 0)
        fd = sys_open(argv[1], 1);
      if (fd < 0)
        uputs("touch: failed\n");
      else
        sys_close(fd);
    }
  } else if (strcmp(argv[0], "rm") == 0) {
    if (argc < 2) {
      uputs("rm: missing file name\n");
    } else {
      if (sys_unlink(argv[1]) < 0)
        uputs("rm: failed\n");
    }
  } else if (strcmp(argv[0], "mv") == 0) {
    if (argc < 3) {
      uputs("mv: usage: mv SRC DST\n");
    } else if (strcmp(argv[1], argv[2]) == 0) {
      // nothing to do if source and destination are the same
      return;
    } else {
      int srcfd = sys_open(argv[1], 0);
      if (srcfd < 0) {
        uputs("mv: cannot open source file\n");
      } else {
        // prepare destination: if it exists, remove it, then create a new file
        (void)sys_unlink(argv[2]);
        int dstfd = sys_open(argv[2], 1);
        if (dstfd < 0) {
          uputs("mv: cannot open destination file\n");
          sys_close(srcfd);
        } else {
          char buf[128];
          while (1) {
            long r = sys_read(srcfd, buf, sizeof(buf));
            if (r <= 0)
              break;
            sys_write(dstfd, buf, (uint64_t)r);
          }
          sys_close(srcfd);
          sys_close(dstfd);
          if (sys_unlink(argv[1]) < 0) {
            uputs("mv: warning: failed to remove source\n");
          }
        }
      }
    }
  } else if (strcmp(argv[0], "pwd") == 0) {
    // file system currently only supports a single root directory
    uputs("/\n");
  } else if (strcmp(argv[0], "mkdir") == 0) {
    uputs("mkdir: directories are not supported (flat filesystem)\n");
  } else if (strcmp(argv[0], "rmdir") == 0) {
    uputs("rmdir: directories are not supported (flat filesystem)\n");
  } else if (strcmp(argv[0], "write") == 0) {
    extern int pipe_input_active;
    extern const char *pipe_input_name;
    if (argc < 3 && !(argc == 2 && pipe_input_active)) {
      uputs("write: usage: write FILE TEXT...\n");
    } else if (argc >= 3) {
      // concatenate all args after filename with spaces (no libc strcat)
      char buf[256];
      int pos = 0;
      for (int i = 2; i < argc; i++) {
        const char *s = argv[i];
        // add space between args
        if (i > 2) {
          if (pos + 1 >= (int)sizeof(buf))
            break;
          buf[pos++] = ' ';
        }
        size_t slen = strlen(s);
        if (pos + (int)slen >= (int)sizeof(buf))
          slen = (size_t)((int)sizeof(buf) - 1 - pos);
        for (size_t k = 0; k < slen; k++)
          buf[pos++] = s[k];
        if (pos >= (int)sizeof(buf) - 1)
          break;
      }
      buf[pos] = '\0';
      // To implement overwrite semantics, first try truncating file to size=0;
      // if the file does not exist, this call will fail and we will create it later.
      if (sys_trunc(argv[1]) < 0) {
        // ignore, may not exist yet
      }
      int fd = sys_open(argv[1], 0);
      if (fd < 0)
        fd = sys_open(argv[1], 1);
      if (fd < 0) {
        uputs("write: cannot open file\n");
      } else {
        sys_write(fd, buf, strlen(buf));
        sys_close(fd);
      }
    } else { // argc == 2 && pipe_input_active: read from pipe temp file and write into target
      char buf[256];
      int in = sys_open(pipe_input_name, 0);
      if (in < 0) {
        uputs("write: cannot open pipe input\n");
        return;
      }
      long r = sys_read(in, buf, sizeof(buf) - 1);
      sys_close(in);
      if (r <= 0) {
        uputs("write: empty pipe input\n");
        return;
      }
      buf[r] = '\0';

      if (sys_trunc(argv[1]) < 0) {
        // ignore failure; file may not exist yet
      }
      int fd = sys_open(argv[1], 0);
      if (fd < 0)
        fd = sys_open(argv[1], 1);
      if (fd < 0) {
        uputs("write: cannot open file\n");
      } else {
        sys_write(fd, buf, (uint64_t)r);
        sys_close(fd);
      }
    }
  } else if (strcmp(argv[0], "fork") == 0) {
    int pid = sys_fork();
    if (pid < 0) {
      uputs("fork: failed\n");
    } else if (pid == 0) {
      // child
      uputs("[child] hello from child process\n");
      sys_exit(0);
    } else {
      // parent
      uputs("[parent] forked child pid= ");
      // very simple decimal print
      char numbuf[32];
      int n = pid;
      int idx = 0;
      if (n == 0) {
        numbuf[idx++] = '0';
      } else {
        char tmp[32];
        int t = 0;
        while (n > 0 && t < (int)sizeof(tmp)) {
          tmp[t++] = (char)('0' + (n % 10));
          n /= 10;
        }
        while (t > 0)
          numbuf[idx++] = tmp[--t];
      }
      numbuf[idx] = '\0';
      uputs(numbuf);
      uputc('\n');

      // ensure parent waits for child to finish so that shell
      // continues predictably and we exercise the tested wait path
      sys_wait();
    }
  } else if (strcmp(argv[0], "bg") == 0) {
    int pid = sys_fork();
    if (pid < 0) {
      uputs("bg: fork failed\n");
    } else if (pid == 0) {
      // child: background worker that suspends itself into blocked_list
      uputs("[bg] background worker started\n");
      sys_suspend(); // never returns
      sys_exit(0);
    } else {
      // parent: do not wait, just report
      uputs("[bg] started background process pid= ");
      char numbuf[32];
      int n = pid;
      int idx = 0;
      if (n == 0) {
        numbuf[idx++] = '0';
      } else {
        char tmp[32];
        int t = 0;
        while (n > 0 && t < (int)sizeof(tmp)) {
          tmp[t++] = (char)('0' + (n % 10));
          n /= 10;
        }
        while (t > 0)
          numbuf[idx++] = tmp[--t];
      }
      numbuf[idx] = '\0';
      uputs(numbuf);
      uputc('\n');
    }
  } else if (strcmp(argv[0], "kill") == 0) {
    if (argc < 2) {
      uputs("kill: usage: kill PID\n");
    } else {
      // simple decimal parsing
      const char *s = argv[1];
      int neg = 0;
      if (*s == '-') {
        neg = 1;
        s++;
      }
      int pid = 0;
      while (*s) {
        if (*s < '0' || *s > '9') {
          uputs("kill: invalid pid\n");
          return;
        }
        pid = pid * 10 + (*s - '0');
        s++;
      }
      if (neg)
        pid = -pid;

      int r = sys_kill(pid);
      if (r < 0)
        uputs("kill: no such process or cannot kill\n");
    }
  } else if (strcmp(argv[0], "help") == 0) {
    cmd_help();
  } else {
    // not a built-in: try xv6-style fork+exec
    int pid = sys_fork();
    if (pid < 0) {
      uputs("fork: failed\n");
    } else if (pid == 0) {
      // child: replace image with named program
      if (sys_exec(argv[0]) < 0) {
        uputs("exec: failed\n");
        sys_exit(1);
      }
    } else {
      // parent: wait for child to finish
      sys_wait();
    }
  }
}

// shell entry function, will be used as process entrypoint
void user_shell(void) {
  char line[256];
  char *argv[MAX_ARGS];

  (void)sys_getpid(); // touch to avoid unused warning

  uputs(MAGENTA "Welcome to Lrix shell! Type 'help' for help." RESET "\n");

  while (1) {
    uprompt("root", "Lrix");
    int len = readline(line, sizeof(line));
    if (len <= 0)
      continue;
    // check for simple pipeline: cmd1 | cmd2
    char *pipe_pos = 0;
    for (int i = 0; line[i]; i++) {
      if (line[i] == '|') {
        pipe_pos = &line[i];
        break;
      }
    }

    if (!pipe_pos) {
      int argc = parse_args(line, argv, MAX_ARGS);
      execute(argc, argv);
    } else {
      *pipe_pos = '\0';
      char *left = trim_spaces(line);
      char *right = trim_spaces(pipe_pos + 1);

      if (*left == '\0' || *right == '\0') {
        uputs("pipe: invalid syntax\n");
        continue;
      }

      // first run left side, redirecting shell output to temp file
      char *argv1[MAX_ARGS];
      char *argv2[MAX_ARGS];
      int argc1 = parse_args(left, argv1, MAX_ARGS);
      int argc2 = parse_args(right, argv2, MAX_ARGS);
      if (argc1 == 0 || argc2 == 0) {
        uputs("pipe: invalid commands\n");
        continue;
      }

      // prepare temp file for pipe output
      sys_trunc(pipe_input_name);
      // Prefer opening without create; if it does not exist, create it to avoid fs_create failure
      int pfd = sys_open(pipe_input_name, 0);
      if (pfd < 0)
        pfd = sys_open(pipe_input_name, 1);
      if (pfd < 0) {
        uputs("pipe: cannot open temp file\n");
        continue;
      }
      shell_out_fd = pfd;
      execute(argc1, argv1);
      shell_out_fd = -1;
      sys_close(pfd);

      // second run right side, letting cmd_cat/read/write etc. read from the pipe temp file
      pipe_input_active = 1;
      execute(argc2, argv2);
      pipe_input_active = 0;
    }
  }
}
