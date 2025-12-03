.section .text
.global trap_vector_entry
.type trap_vector_entry, @function

/* simple trap entry: save a few registers, then call the C function trap_handler_c */
.align 2  ;
trap_vector_entry:
    /* Allocate stack frame (enough to save registers) */
    addi sp, sp, -128

    /* Save caller-saved / callee-saved registers if printing registers is needed */
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

    /* return to the instruction pointed by mepc/mode via mret */
    mret
