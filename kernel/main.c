// main.c - 内核主函数
#include "types.h"
#include "uart.h"
// 提前声明printk函数
#include "color.h"

// 内核主函数
int kmain() {
    // 初始化控制台
    uart_init();
    // 测试printk函数
    INFO("welcome to MiniOS");
    // 进入主循环
    while (1) {
        // 内核主循环
    }
}
