# Makefile - RISC-V内核构建

CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -I. -Wall -O2 -mcmodel=medany
LDFLAGS = -T linker.ld -nostdlib

OBJS = start.o \
	   main.o \
	   uart.o

all: kernel.bin

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin

run: kernel.bin
	qemu-system-riscv64 -machine virt -kernel kernel.bin -nographic -bios none

.PHONY: all clean run
