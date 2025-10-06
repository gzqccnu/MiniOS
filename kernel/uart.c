// uart.c - printk实现 (QEMU virt / 16550 串口)

#include "types.h"
#include "uart.h"
#include "color.h"

// NS16550-compatible registers at 0x10000000 (QEMU virt typical)
#define UART_BASE 0x10000000
#define UART_RBR  (UART_BASE + 0x00)  // Receiver Buffer (read)
#define UART_THR  (UART_BASE + 0x00)  // Transmitter Holding (write)
#define UART_IER  (UART_BASE + 0x01)  // Interrupt Enable
#define UART_IIR  (UART_BASE + 0x02)  // Interrupt ID / FIFO control
#define UART_LCR  (UART_BASE + 0x03)  // Line Control
#define UART_MCR  (UART_BASE + 0x04)  // Modem control
#define UART_LSR  (UART_BASE + 0x05)  // Line Status // data ready bit0 (0x01) // empty bit5 (0x20)
#define UART_DLL  (UART_BASE + 0x00)  // Divisor Latch Low (when LCR[7]=1)
#define UART_DLM  (UART_BASE + 0x01)  // Divisor Latch High (when LCR[7]=1)

// 等待并写入字符到 THR
static void uart_putc(char c) {
    volatile unsigned char *lsr = (volatile unsigned char *)UART_LSR;
    volatile unsigned char *thr = (volatile unsigned char *)UART_THR;

    // 等待 THR 可写：LSR 的 bit5 (0x20) = Transmitter Holding Register Empty
    while (!(*lsr & 0x20)) {
        // busy-wait
    }
    
    *thr = (unsigned char)c;
}

// 读取字符（非阻塞/简单实现）
static char uart_getc(void) {
    volatile unsigned char *rbr = (volatile unsigned char *)UART_RBR;
    volatile unsigned char *lsr = (volatile unsigned char *)UART_LSR;

    if (*lsr & 0x01) { // Data Ready
        return (char)(*rbr);
    }
    return 0;
}

// 初始化 uart：设置 8N1（不强制设置波特，使用 QEMU 默认）
void uart_init(void) {
    volatile unsigned char *lcr = (volatile unsigned char *)UART_LCR;
    // 设置 8 bits, no parity, 1 stop (0x03)
    *lcr = 0x03;
    INFO("waiting for uart init...");
    SUCCESS("uart init success");
    // 不强行设置 DLL/DLM（QEMU 默认 115200），如果需要特定波特率，可：
    // - 设置 LCR 的 DLAB 位（bit7）=1，然后写 DLL/DLM
    // 这里保持简单：不修改波特
}

// 简单字符串输出，遇到 '\n' 输出 "\r\n"
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
// 简单的printk实现 (支持 %d %s %x %c)
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
                char *str = va_arg(args, char*);
                if (!str) str = "(null)";
                while (*str) {
                    *p++ = *str++;
                }
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
                while (*h == '0' && *(h+1)) h++;
                while (*h) *p++ = *h++;
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
