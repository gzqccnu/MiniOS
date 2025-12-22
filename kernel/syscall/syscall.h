// syscall.h - syscall numbers and dispatcher prototype
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>

/* syscall numbers */
#define SYS_EXIT 1
#define SYS_GETPID 2
#define SYS_FORK 3
#define SYS_WAIT 4
#define SYS_SBRK 5
#define SYS_SLEEP 6
#define SYS_KILL 7
#define SYS_UPTIME 8
#define SYS_WRITE 9
#define SYS_OPEN 10
#define SYS_READ 11
#define SYS_CLOSE 12
// list root directory entries
#define SYS_LS 13
// read a single character from console (UART), blocking
#define SYS_GETC 14
// unlink (remove) a file in root directory
#define SYS_UNLINK 15
// exec: replace current process image with named user program
#define SYS_EXEC 16
// truncate file (set size to 0)
#define SYS_TRUNC 17

// list processes (ps)
#define SYS_PS 18

/* dispatcher: num, args[6], epc -> return value */
uint64_t syscall_dispatch(uint64_t num, uint64_t args[6], uint64_t epc);

// exec helper: map program name (in args[0]) to entry address, or -1 on failure
uint64_t sys_exec_lookup(uint64_t args[6]);

#endif /* _SYSCALL_H_ */
