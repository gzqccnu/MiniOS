/*
 * MiniOS
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 * 
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 * 
 * Project homepage: https://github.com/lrisguan/MiniOS
 * Description: A scratch implemention of OS based on RISC-V
 */

// uart.c - printk implementation (QEMU virt / 16550 UART)

#include "uart.h"
#include "../include/log.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include <stdarg.h>
#include <stdint.h>

// NS16550-compatible registers at 0x10000000 (QEMU virt typical)
#define UART_BASE 0x10000000
#define UART_RBR (UART_BASE + 0x00) // Receiver Buffer (read)
#define UART_THR (UART_BASE + 0x00) // Transmitter Holding (write)
#define UART_IER (UART_BASE + 0x01) // Interrupt Enable
#define UART_IIR (UART_BASE + 0x02) // Interrupt ID / FIFO control
#define UART_LCR (UART_BASE + 0x03) // Line Control
#define UART_MCR (UART_BASE + 0x04) // Modem control
#define UART_LSR (UART_BASE + 0x05) // Line Status // data ready bit0 (0x01) // empty bit5 (0x20)
#define UART_DLL (UART_BASE + 0x00) // Divisor Latch Low (when LCR[7]=1)
#define UART_DLM (UART_BASE + 0x01) // Divisor Latch High (when LCR[7]=1)

// Wait and write character to THR
static void uart_putc(char c) {
  volatile unsigned char *lsr = (volatile unsigned char *)UART_LSR;
  volatile unsigned char *thr = (volatile unsigned char *)UART_THR;

  // Wait for THR to be writable: LSR's bit5 (0x20) = Transmitter Holding Register Empty
  while (!(*lsr & 0x20)) {
    // busy-wait
  }

  *thr = (unsigned char)c;
}

// Read character (non-blocking/simple implementation)
static char uart_getc(void) {
  volatile unsigned char *rbr = (volatile unsigned char *)UART_RBR;
  volatile unsigned char *lsr = (volatile unsigned char *)UART_LSR;

  if (*lsr & 0x01) { // Data Ready
    return (char)(*rbr);
  }
  return 0;
}

// Initialize uart: set to 8N1 (don't force baud rate, use QEMU default)
void uart_init(void) {
  volatile unsigned char *lcr = (volatile unsigned char *)UART_LCR;
  // Set 8 bits, no parity, 1 stop (0x03)
  *lcr = 0x03;
  // INFO("waiting for uart init...");
  // SUCCESS("uart init success");
}

// Simple string output, output "\r\n" when encountering '\n'
void puts(const char *s) {
  while (*s) {
    if (*s == '\n') {
      uart_putc('\r');
    }
    uart_putc(*s++);
  }
}

/* Blocking read one char from UART (returns unsigned char value) */
char uart_getc_blocking(void) {
  char c = 0;
  while ((c = uart_getc()) == 0) {
    ;
  }
  return c;
}

/*
 * Added: Read and echo characters
 * Only by calling this function to read input can the user's keystrokes be seen in the terminal.
 */
static char uart_getc_echo(void) {
  char c = uart_getc_blocking();

  // echo
  if (c == '\r') {
    /*
     * If it's a carriage return, echo the carriage return and newline so that
     * the cursor moves to the beginning of the next line.
     */
    uart_putc('\r');
    uart_putc('\n');
  } else if (c == 127 || c == '\b') {
    /*
     * Simple backspace handling (visually deleted, logically determined
     * by upper-level parsing)
     */
    uart_putc('\b');
    uart_putc(' ');
    uart_putc('\b');
  } else {
    uart_putc(c);
  }
  return c;
}

/*
 * Read a line from UART into buf (maxlen includes trailing NUL).
 * Stops on \n or \r, returns number of characters (excluding NUL).
 */
int uart_getline(char *buf, int maxlen) {
  if (!buf || maxlen <= 1)
    return 0;
  int i = 0;
  while (i < maxlen - 1) {
    char c = uart_getc_echo();
    if (c == '\r' || c == '\n')
      break;
    buf[i++] = c;
  }
  buf[i] = '\0';
  return i;
}

static int is_space(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}
static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* parse integer decimal from UART into *out; return 1 on success */
static int uart_read_int(int *out) {
  int sign = 1;
  int val = 0;

  char c = uart_getc_echo();
  while (is_space(c))
    c = uart_getc_echo();

  if (c == '+' || c == '-') {
    if (c == '-')
      sign = -1;
    c = uart_getc_echo();
  }

  if (!is_digit(c))
    return 0;

  while (is_digit(c)) {
    val = val * 10 + (c - '0');
    // read the next char
    c = uart_getc_echo();
  }
  *out = val * sign;
  return 1;
}

/* parse unsigned long decimal */
static int uart_read_ulong(unsigned long *out) {
  unsigned long val = 0;
  char c = uart_getc_echo();
  while (is_space(c))
    c = uart_getc_echo();

  if (!is_digit(c))
    return 0;

  while (is_digit(c)) {
    val = val * 10 + (c - '0');
    c = uart_getc_echo();
  }
  *out = val;
  return 1;
}

/* parse hex number into uintptr_t */
static int uart_read_hex(uintptr_t *out) {
  uintptr_t val = 0;
  char c = uart_getc_echo();
  while (is_space(c))
    c = uart_getc_echo();

  /* optional 0x */
  if (c == '0') {
    // At this point, the screen has already echoed '0'
    // Read the next character to check if it is 'x'
    char c2 = uart_getc_echo();
    if (c2 == 'x' || c2 == 'X') {
      // It's 0x, continue reading the next digit
      c = uart_getc_echo();
    } else {
      // Not x, for example if the input is "0123", c2 is '1'.
      // We have already consumed c2 (and echoed it), so treat c2 as the current character
      c = c2;
    }
  }

  if (!is_hex_digit(c))
    return 0;

  while (is_hex_digit(c)) {
    int d;
    if (c >= '0' && c <= '9')
      d = c - '0';
    else if (c >= 'a' && c <= 'f')
      d = 10 + (c - 'a');
    else
      d = 10 + (c - 'A');
    val = (val << 4) | (uintptr_t)d;

    c = uart_getc_echo();
  }
  *out = val;
  return 1;
}

/* parse string (read until whitespace), write NUL-terminated into buf */
static int uart_read_string(char *buf) {
  char c = uart_getc_echo();
  while (is_space(c))
    c = uart_getc_echo();

  int i = 0;
  // Read until a whitespace character (such as space or newline)
  while (c && !is_space(c)) {
    buf[i++] = c;
    c = uart_getc_echo();
  }
  buf[i] = '\0';
  return 1;
}

/* scank: minimal scanf-like function reading from UART
 * Supports formats: %d %s %p %x %c %lu
 */
int scank(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int assigned = 0;
  const char *p = fmt;

  while (*p) {
    if (*p != '%') {
      /* consume matching literal from input if not whitespace; skip whitespace in format */
      if (is_space(*p)) {
        /* skip any whitespace in input */
        char c = uart_getc_echo();
        while (is_space(c)) {
          c = uart_getc_echo();
        }
      }
      p++;
      continue;
    }

    p++; /* skip % */
    if (*p == '\0')
      break;

    switch (*p) {
    case 'd': {
      int *ip = va_arg(args, int *);
      if (uart_read_int(ip))
        assigned++;
      break;
    }
    case 'l': {
      /* expect lu */
      p++;
      if (*p == 'u') {
        unsigned long *ulp = va_arg(args, unsigned long *);
        if (uart_read_ulong(ulp))
          assigned++;
      }
      break;
    }
    case 'x': {
      unsigned int *xp = va_arg(args, unsigned int *);
      uintptr_t tmp;
      if (uart_read_hex(&tmp)) {
        *xp = (unsigned int)tmp;
        assigned++;
      }
      break;
    }
    case 'p': {
      uintptr_t *pp = va_arg(args, uintptr_t *);
      uintptr_t tmp;
      if (uart_read_hex(&tmp)) {
        *pp = tmp;
        assigned++;
      }
      break;
    }
    case 's': {
      char *sp = va_arg(args, char *);
      if (uart_read_string(sp))
        assigned++;
      break;
    }
    case 'c': {
      char *cp = va_arg(args, char *);
      char c = uart_getc_echo();
      *cp = c;
      assigned++;
      break;
    }
    default:
      /* unsupported, skip */
      break;
    }
    p++;
  }
  va_end(args);
  return assigned;
}

// Simple printk implementation (supports %d %s %x %c %p %lu)
void printk(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char buffer[256];
  char *p = buffer;
  char c;

  while ((c = *fmt++) != '\0') {
    if (c != '%') {
      *p++ = c;
      continue;
    }

    c = *fmt++;
    switch (c) {
    case 'd': {
      int num = va_arg(args, int);
      if (num == 0) {
        *p++ = '0';
        break;
      }
      if (num < 0) {
        *p++ = '-';
        num = -num;
      }
      char num_buf[16];
      char *n = num_buf;
      while (num > 0) {
        *n++ = '0' + (num % 10);
        num /= 10;
      }
      while (n > num_buf) {
        *p++ = *--n;
      }
      break;
    }
    case 's': {
      char *str = va_arg(args, char *);
      if (!str)
        str = "(null)";
      while (*str) {
        *p++ = *str++;
      }
      break;
    }
    case 'p': {
      *p++ = '0';
      *p++ = 'x';
      uintptr_t num = va_arg(args, uintptr_t);
      char hex_buf[sizeof(uintptr_t) * 2 + 1];
      hex_buf[sizeof(uintptr_t) * 2] = '\0';
      for (int i = sizeof(uintptr_t) * 2 - 1; i >= 0; --i) {
        int d = num & 0xF;
        hex_buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
        num >>= 4;
      }
      char *h = hex_buf;
      while (*h)
        *p++ = *h++;
      break;
    }
    case 'x': {
      unsigned int num = va_arg(args, unsigned int);
      char hex_buf[9];
      hex_buf[8] = '\0';
      for (int i = 7; i >= 0; --i) {
        int d = num & 0xF;
        hex_buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
        num >>= 4;
      }
      char *h = hex_buf;
      while (*h == '0' && *(h + 1))
        h++;
      while (*h)
        *p++ = *h++;
      break;
    }
    case 'l': {
      if (*fmt == 'u') {
        fmt++;
        unsigned long num = va_arg(args, unsigned long);
        if (num == 0) {
          *p++ = '0';
          break;
        }
        char num_buf[21];
        char *n = num_buf;
        while (num > 0) {
          *n++ = '0' + (num % 10);
          num /= 10;
        }
        while (n > num_buf) {
          *p++ = *--n;
        }
      }
      break;
    }
    case 'c': {
      char ch = (char)va_arg(args, int);
      *p++ = ch;
      break;
    }
    default:
      *p++ = c;
      break;
    }
  }

  *p = '\0';
  puts(buffer);
  va_end(args);
}
