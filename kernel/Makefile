# ==========================================================
# Makefile - RISC-V Kernel (Selective build + build dir)
# ==========================================================

# ---------- Toolchain ----------
CC       := riscv64-unknown-elf-gcc
LD       := riscv64-unknown-elf-ld
OBJCOPY  := riscv64-unknown-elf-objcopy

# ---------- Directories ----------
# all directory（default）
ALL_SRC_DIRS := boot trap uart string proc mem  
# compilation output directory
BUILD_DIR ?= build
# use 
# make DIR="dir1 dir2" 
# to compile specific directory
# and use
# make DIR="dir1 dir2 ..." run
# to run the system
DIR ?= all

# ---------- Include paths ----------
INC_DIRS := include .
CFLAGS   := -Wall -O2 -nostdlib -fno-builtin -fno-stack-protector \
            -mcmodel=medany $(addprefix -I, $(INC_DIRS))

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
	qemu-system-riscv64 -machine virt -kernel $(BIN) -nographic -bios none

# Show info
info:
	@echo "DIR        = $(DIR)"
	@echo "SRC_DIRS   = $(SRC_DIRS)"
	@echo "C_SRCS     = $(C_SRCS)"
	@echo "S_SRCS     = $(S_SRCS)"
	@echo "OBJS       = $(OBJS)"

# Clean
clean:
	@echo "  [CLEAN]"
	rm -rf $(BUILD_DIR)

# ==========================================================
