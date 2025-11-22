.section .text
.global trap_vector_entry
.type trap_vector_entry, @function

/* 简单 trap 入口：保存少量寄存器，然后调用 C 函数 trap_handler_c */
.align 2  ;
trap_vector_entry:
    addi sp, sp, -128        /* 分配栈帧（足够保存寄存器） */

    /* 保存 caller-saved / callee-saved 若需要打印寄存器 */
    sd ra, 0(sp)
    sd t0, 8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)
    sd a0, 32(sp)
    sd a1, 40(sp)
    sd a2, 48(sp)
    sd a3, 56(sp)
    sd a4, 64(sp)
    sd a5, 72(sp)
    sd a6, 80(sp)
    sd a7, 88(sp)

    /* call C handler */
    call trap_handler_c

    /* restore registers */
    ld ra, 0(sp)
    ld t0, 8(sp)
    ld t1, 16(sp)
    ld t2, 24(sp)
    ld a0, 32(sp)
    ld a1, 40(sp)
    ld a2, 48(sp)
    ld a3, 56(sp)
    ld a4, 64(sp)
    ld a5, 72(sp)
    ld a6, 80(sp)
    ld a7, 88(sp)

    addi sp, sp, 128

    /* 在发生异常后我们通常不返回 mret（避免反复陷入），转入死循环 */
1:
    wfi
    j 1b
