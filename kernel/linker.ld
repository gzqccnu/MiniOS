/* linker.ld - RISC-V内核链接器脚本*/

ENTRY(_start)

/* 定义内存布局常量 */
MEMORY {
    /* QEMU virt 机器默认有 128MB RAM */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* 定义栈和堆的大小 */
STACK_SIZE = 8K;      /* 8KB 栈 */
HEAP_SIZE = 16M;       /* 1MB 堆 - 根据需要调整 */

SECTIONS {
    /* 内核代码和数据从 RAM 的起始位置开始 */
    . = ORIGIN(RAM);

    .text : {
        *(.text.init)
        *(.text .text.*)
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
    } > RAM

    .data : {
        *(.data .data.*)
    } > RAM

    /* .bss 节，内核的未初始化数据 */
    .bss : {
        /* 对齐到 8 字节边界 */
        . = ALIGN(8);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > RAM

    /*
     * 堆的定义:
     * 堆在 .bss 节之后，大小固定为 HEAP_SIZE
     */
    . = ALIGN(0x1000);  /* 确保堆起始地址对齐 */
    _heap_start = .;
    . = . + HEAP_SIZE;
    . = ALIGN(0x1000);
    _heap_end = .;

    /*
     * 栈的定义:
     * 栈在堆之后，向上增长（或者你可以改为向下增长）
     */
    . = ALIGN(16);
    _stack_bottom = .;
    . = . + STACK_SIZE;
    _stack_top = .;

    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.comment)
    }
}