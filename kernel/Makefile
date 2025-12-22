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
C_SRCS   := $(shell find $(SRC_DIRS) -name "*.c" 2>/dev/null)
S_SRCS   := $(shell find $(SRC_DIRS) -name "*.S" 2>/dev/null)
OBJS     := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o) $(S_SRCS:.S=.o))
OBJS     += $(BUILD_DIR)/main.o

# ---------- Rules ----------
.PHONY: all clean run info dirs

all: dirs $(BIN)

dirs:
	@mkdir -p $(BUILD_DIR)
	@for d in $(SRC_DIRS); do mkdir -p $(BUILD_DIR)/$$d; done

# Build ELF
$(ELF): $(OBJS) linker.ld
	@echo "  [LD]  $@"
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Build BIN
$(BIN): $(ELF)
	@echo "  [OBJCOPY]  $@"
	$(OBJCOPY) -O binary $< $@
	@echo $(VIRTIO) > $(BUILD_DIR)/.virtio

# Compile C
$(BUILD_DIR)/%.o: %.c
	@echo "  [CC]  $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly
$(BUILD_DIR)/%.o: %.S
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
	@# Also clean user-space objects produced from ./usr sources
	rm -rf ./usr

# ==========================================================
