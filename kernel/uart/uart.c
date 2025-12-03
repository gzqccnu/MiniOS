// uart.c - printk implementation (QEMU virt / 16550 UART)

#include "uart.h"
#include "../include/log.h"
#include "../include/types.h"
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
  INFO("waiting for uart init...");
  SUCCESS("uart init success");
  // Don't forcibly set DLL/DLM (QEMU default is 115200). If a specific baud rate is needed, you can:
  // - Set the DLAB bit of LCR (bit7) to 1, then write DLL/DLM
  // Keep it simple here: don't modify baud rate
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

void gets(const char *s) {
  while (*s) {
    if (*s == '\n') {
      uart_getc();
    }
  }
}
// Simple printk implementation (supports %d %s %x %c)
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
      // Add "0x" prefix for pointer address, this is standard practice
      *p++ = '0';
      *p++ = 'x';

      // Treat the pointer as uintptr_t (an unsigned integer large enough to hold a pointer)
      // then print it like handling '%x'
      uintptr_t num = va_arg(args, uintptr_t);

      // Use a buffer large enough to handle 32-bit or 64-bit addresses
      char hex_buf[sizeof(uintptr_t) * 2 + 1];
      hex_buf[sizeof(uintptr_t) * 2] = '\0';

      // Fill the buffer from the end
      for (int i = sizeof(uintptr_t) * 2 - 1; i >= 0; --i) {
        int d = num & 0xF;
        hex_buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
        num >>= 4;
      }

      // Copy the result to the main buffer, can skip leading zeros, but for addresses, it's common to display all of them
      char *h = hex_buf;
      // while (*h == '0' && *(h+1)) h++; // For addresses, we usually want to see the full length
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
      // skip leading zeros
      char *h = hex_buf;
      while (*h == '0' && *(h + 1))
        h++;
      while (*h)
        *p++ = *h++;
      break;
    }
    case 'l': {
      if (*fmt == 'u') { // Check if it is "lu"
        fmt++;           // Skip 'u'

        // [Key Fix] use unsigned long to get the argument
        unsigned long num = va_arg(args, unsigned long);

        if (num == 0) {
          *p++ = '0';
          break;
        }

        char num_buf[21]; // 64-bit unsigned integer has at most 20 digits + null
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

    // Handle long type specifically
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
