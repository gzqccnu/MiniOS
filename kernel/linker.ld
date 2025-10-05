/* linker.ld - RISC-V内核链接器脚本 */

ENTRY(_start)

SECTIONS {
    . = 0x80000000; /* QEMU virt机器内核加载地址 */

    .text : {
        *(.text.init)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /* 添加栈定义 */
    .stack : {
        _stack_bottom = .;
        . += 8192; /* 8KB栈空间 */
        _stack_top = .;
    }

    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.comment)
    }
}
