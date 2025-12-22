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

// uart.h

#ifndef _UART_H_
#define _UART_H_

void uart_init(void);

void puts(const char *s);

// blocking read one character from UART
char uart_getc_blocking(void);

// read a line into buf (NUL-terminated), return length (without NUL)
int uart_getline(char *buf, int maxlen);

void printk(const char *fmt, ...);
int scank(const char *fmt, ...);

#endif // _UART_H_
