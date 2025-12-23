# MiniOS
# Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
# 
# This program is released under the terms of the GNU General Public License version 2(GPLv2).
# See https://opensource.org/licenses/GPL-2.0 for more information.
# 
# Project homepage: https://github.com/lrisguan/MiniOS
# Description: A scratch implemention of OS based on RISC-V

# ==========================================================
# Makefile - RISC-V Kernel (Selective build + build dir)
# ==========================================================

# ---------- Toolchain ----------
CC       := riscv64-unknown-elf-gcc
LD       := riscv64-unknown-elf-ld
OBJCOPY  := riscv64-unknown-elf-objcopy

# ---------- Directories ----------
# all directory（default）
# Additionally add ../usr to link the user-space shell into the kernel image as 
# the first process
ALL_SRC_DIRS := boot trap uart string proc mem syscall fs ../usr 
# compilation output directory
BUILD_DIR ?= build
# use 
# make DIR="dir1 dir2" 
# to compile specific directory
# and use
# make DIR="dir1 dir2 ..." run
# to run the system

CFLAGS   = -Wall -O2 -nostdlib -fno-builtin -fno-stack-protector \
			-mcmodel=medany $(addprefix -I, $(INC_DIRS)) \
			-DTRAP_DEBUG=$(TRAP_DEBUG)
			
DIR ?= all

TRAP_DEBUG ?= 0
# FS_DEBUG: control filesystem debug logs; default off; set FS_DEBUG=1 to enable
FS_DEBUG   ?= 0

ifeq ($(FS_DEBUG),1)
	CFLAGS += -DFS_DEBUG
endif

# VIRTIO Version Selection:
# 1) Specify on the command line: make VIRTIO=1 ... / make VIRTIO=2 ...
# 2) Otherwise: if build/.virtio exists, use the VIRTIO from the last build
# 3) If not, default to 1
VIRTIO ?= $(shell if [ -f $(BUILD_DIR)/.virtio ]; then cat $(BUILD_DIR)/.virtio; else echo 1; fi)

# Kernel macros determined based on VIRTIO
ifeq ($(VIRTIO), 1)
    CFLAGS += -D_VIRTIO_FORCE_V1
else ifeq ($(VIRTIO), 2)
    CFLAGS += -D_VIRTIO_FORCE_V2
endif

QEMU_OPTS = -machine virt -nographic -bios none -kernel build/kernel.bin \
            -drive if=none,format=raw,file=disk.img,id=hd0
ifeq ($(VIRTIO), 1)
    # Tell QEMU to forcibly emulate a V1 (Legacy) device
    QEMU_OPTS += -global virtio-mmio.force-legacy=true
    QEMU_OPTS += -device virtio-blk-device,drive=hd0
else ifeq ($(VIRTIO), 2)
	# Modern mode: explicitly disable legacy, enable MMIO version 2
	QEMU_OPTS += -global virtio-mmio.force-legacy=false
	QEMU_OPTS += -device virtio-blk-device,drive=hd0
else
    # Auto mode (default): do not force legacy, let QEMU decide
    QEMU_OPTS += -device virtio-blk-device,drive=hd0
endif



# ---------- Include paths ----------
INC_DIRS := include .

LDFLAGS  := -T linker.ld -nostdlib

# ---------- Targets ----------
TARGET   := kernel
ELF      := $(BUILD_DIR)/$(TARGET).elf
BIN      := $(BUILD_DIR)/$(TARGET).bin

# ---------- Source selection ----------
ifeq ($(DIR),all)
    SRC_DIRS := $(ALL_SRC_DIRS)
else
    # only compile specified directories (check if they exist)
    ifneq ($(wildcard $(DIR)),)
        SRC_DIRS := $(DIR)
    else
        $(error Directory '$(DIR)' does not exist!)
    endif
endif

# ---------- Source & Object files ----------
# Collect all C/Assembly sources (including ../usr)
C_SRCS   := $(shell find $(SRC_DIRS) -name "*.c" 2>/dev/null)
S_SRCS   := $(shell find $(SRC_DIRS) -name "*.S" 2>/dev/null)

# Split kernel sources and user-space sources (under ../usr)
KERN_C_SRCS := $(filter-out ../usr/%,$(C_SRCS))
USR_C_SRCS  := $(filter ../usr/%,$(C_SRCS))
KERN_S_SRCS := $(filter-out ../usr/%,$(S_SRCS))
USR_S_SRCS  := $(filter ../usr/%,$(S_SRCS))

# Kernel object files live directly under $(BUILD_DIR)/... (mirroring source tree)
KERN_OBJS := $(addprefix $(BUILD_DIR)/, $(KERN_C_SRCS:.c=.o) $(KERN_S_SRCS:.S=.o))

# User-space objects from ../usr are placed under $(BUILD_DIR)/usr/...
USR_OBJS  := $(addprefix $(BUILD_DIR)/usr/, \
	$(USR_C_SRCS:../usr/%.c=%.o) \
	$(USR_S_SRCS:../usr/%.S=%.o))

OBJS     := $(KERN_OBJS) $(USR_OBJS)
OBJS     += $(BUILD_DIR)/main.o

# ---------- Rules ----------
.PHONY: all clean run info dirs

all: dirs $(BIN)

dirs:
	@mkdir -p $(BUILD_DIR)
	@# create kernel subdirs (exclude ../usr, which goes to $(BUILD_DIR)/usr)
	@for d in $(filter-out ../usr,$(SRC_DIRS)); do mkdir -p $(BUILD_DIR)/$$d; done
	@mkdir -p $(BUILD_DIR)/usr

# Build ELF
$(ELF): $(OBJS) linker.ld
	@echo "  [LD]  $@"
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Build BIN
$(BIN): $(ELF)
	@echo "  [OBJCOPY]  $@"
	$(OBJCOPY) -O binary $< $@
	@echo $(VIRTIO) > $(BUILD_DIR)/.virtio

# Compile C (kernel sources)
$(BUILD_DIR)/%.o: %.c
	@echo "  [CC]  $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly (kernel sources)
$(BUILD_DIR)/%.o: %.S
	@echo "  [AS]  $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C (user-space sources under ../usr -> $(BUILD_DIR)/usr)
$(BUILD_DIR)/usr/%.o: ../usr/%.c
	@echo "  [CC]  $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly (user-space sources under ../usr -> $(BUILD_DIR)/usr)
$(BUILD_DIR)/usr/%.o: ../usr/%.S
	@echo "  [AS]  $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run with QEMU
run: all
	qemu-system-riscv64 $(QEMU_OPTS)

# Show info
info:
	@echo "---------------------------------------------------------------------------------"
	@echo
	@echo "DIR        = $(DIR)"
	@echo
	@echo "---------------------------------------------------------------------------------"
	@echo
	@echo "SRC_DIRS   = $(SRC_DIRS)"
	@echo
	@echo "---------------------------------------------------------------------------------"
	@echo
	@echo "C_SRCS     = $(C_SRCS)"
	@echo
	@echo "---------------------------------------------------------------------------------"
	@echo
	@echo "S_SRCS     = $(S_SRCS)"
	@echo
	@echo "---------------------------------------------------------------------------------"
	@echo
	@echo "OBJS       = $(OBJS)"
	@echo
	@echo "---------------------------------------------------------------------------------"

# Clean
clean:
	@echo "  [CLEAN]"
	rm -rf $(BUILD_DIR)
	@# user-space sources stay under ../usr; only build outputs under $(BUILD_DIR) are removed

# ==========================================================
