// printk.h - 控制台函数声明

#ifndef _UART_H_
#define _UART_H_

void uart_init(void);
void printk(const char *fmt, ...);
void puts(const char *s);

#endif // _UART_H_
