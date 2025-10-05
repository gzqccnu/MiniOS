# start.S - RISC-V内核启动代码

.section .text.init
.global _start

_start:
    # 设置栈指针
    la sp, _stack_top

    # 清零BSS段
    la a0, _bss_start
    la a1, _bss_end
    bgeu a0, a1, bss_done

bss_loop:
    sd zero, (a0)
    addi a0, a0, 8
    bltu a0, a1, bss_loop

bss_done:
    # 跳转到C主函数
    call kmain
loop:
    j loop
