/*
 * Lrix
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 * 
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 * 
 * Project homepage: https://github.com/lrisguan/Lrix
 * Description: A scratch implemention of OS based on RISC-V
 */
 
/* linker.ld - RISC-V Kernel Linker Script */

ENTRY(_start)

/* Define memory layout constants */
MEMORY {
    /* QEMU virt machine has 128MB RAM by default */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* define size of stack and heap */
STACK_SIZE = 8K;      /* 8KB memory of stack */
HEAP_SIZE = 16M;       /* 1MB heap - adjust as needed */

SECTIONS {
    /* kernel code and data start from the beginning of RAM */
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

    /* .bss section, uninitialized data of the kernel */
    .bss : {
       /* Align to 8-byte boundary */
        . = ALIGN(8);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > RAM

   /*
    * Definition of the heap:
    * The heap is located after the .bss section and has a fixed size of HEAP_SIZE
    */
    . = ALIGN(0x1000); /* ensure heap start address alignment */
    _heap_start = .;
    . = . + HEAP_SIZE;
    . = ALIGN(0x1000);
    _heap_end = .;

    /*
    * Definition of a stack:
    * The stack follows the heap and grows upwards (or you can change it to grow downwards)
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
