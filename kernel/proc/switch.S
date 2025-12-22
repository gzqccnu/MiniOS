# switch.S

    .text
    .align 2
    .globl switch_context
switch_context:
    # a0 = old context pointer
    # a1 = new context pointer

    # --- SAVE OLD CONTEXT ---
    sd x1,  0(a0)   # ra
    sd x5,  8(a0)   # t0
    sd x6, 16(a0)   # t1
    sd x7, 24(a0)   # t2
    sd x8, 32(a0)   # s0/fp
    sd x9, 40(a0)   # s1
    sd x10,48(a0)   # a0
    sd x11,56(a0)   # a1
    sd x12,64(a0)   # a2
    sd x13,72(a0)   # a3
    sd x14,80(a0)   # a4
    sd x15,88(a0)   # a5
    sd x16,96(a0)   # a6
    sd x17,104(a0)  # a7
    sd x18,112(a0)  # s2
    sd x19,120(a0)  # s3
    sd x20,128(a0)  # s4
    sd x21,136(a0)  # s5
    sd x22,144(a0)  # s6
    sd x23,152(a0)  # s7
    sd x24,160(a0)  # s8
    sd x25,168(a0)  # s9
    sd x26,176(a0)  # s10
    sd x27,184(a0)  # s11
    sd x28,192(a0)  # t3
    sd x29,200(a0)  # t4
    sd x30,208(a0)  # t5
    sd x31,216(a0)  # t6

    # save mepc and sp
    csrr t0, mepc
    sd t0, 224(a0)
    sd sp, 232(a0)

    # save mstatus
    csrr t0, mstatus
    sd t0, 240(a0)

    # --- RESTORE NEW CONTEXT ---
    ld x1,  0(a1)   # ra
    ld x5,  8(a1)   # t0
    ld x6, 16(a1)   # t1
    ld x7, 24(a1)   # t2
    ld x8, 32(a1)   # s0/fp
    ld x9, 40(a1)   # s1

    # Note: skip x10 (a0) and x11 (a1), finally restore them

    ld x12,64(a1)   # a2
    ld x13,72(a1)   # a3
    ld x14,80(a1)   # a4
    ld x15,88(a1)   # a5
    ld x16,96(a1)   # a6
    ld x17,104(a1)  # a7
    ld x18,112(a1)  # s2
    ld x19,120(a1)  # s3
    ld x20,128(a1)  # s4
    ld x21,136(a1)  # s5
    ld x22,144(a1)  # s6
    ld x23,152(a1)  # s7
    ld x24,160(a1)  # s8
    ld x25,168(a1)  # s9
    ld x26,176(a1)  # s10
    ld x27,184(a1)  # s11
    ld x28,192(a1)  # t3
    ld x29,200(a1)  # t4
    ld x30,208(a1)  # t5
    ld x31,216(a1)  # t6

    # recover mepc and sp
    ld t0, 224(a1)
    csrw mepc, t0
    ld sp, 232(a1)

    # recover mstatus
    ld t0, 240(a1)
    csrw mstatus, t0

    # finally recover a0 and a1, at this point, there is no need to use the old a1 pointer anymore.
    ld x10, 48(a1)  # a0
    ld x11, 56(a1)  # a1

    ret

.globl forkret
forkret:
    li t0, 0x80      # 0x80 = 1 << 7 (MPIE)
    csrs mstatus, t0 # set MPIE
    mret
