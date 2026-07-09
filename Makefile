# ============================================================
#  MexaOS Build System
#  Intent-Driven Operating System
# ============================================================

# ─── Configuration ───
ARCH        := x86_64
TARGET      := mexaos
VERSION     := 2.0.0
BUILD       := 2240

# ─── Directories ───
BOOT_DIR    := bootloader
KERN_DIR    := kernel
INC_DIR     := include
OUT_DIR     := build
ISO_DIR     := iso

# ─── Tools ───
AS          := nasm
CC          := x86_64-elf-gcc
LD          := x86_64-elf-ld
OBJCOPY     := x86_64-elf-objcopy
QEMU        := qemu-system-x86_64
GRUB_MKRESCUE := grub-mkrescue

# ─── Flags ───
BOOT_ASFLAGS := -f bin
KERN_ASFLAGS := -f elf64 -D__MEXAOS_BUILD__=$(BUILD)

CFLAGS      := -m64 -ffreestanding -O2 -Wall -Wextra \
               -fno-exceptions -fno-rtti -nostdlib -nostartfiles \
               -I$(INC_DIR) -I$(KERN_DIR)/include \
               -DMEXAOS_VERSION=\"$(VERSION)\" \
               -DMEXAOS_BUILD=$(BUILD) \
               -mno-red-zone -mcmodel=large \
               -fno-stack-protector -fno-pic \
               -std=gnu11
LDFLAGS     := -m elf_x86_64 -T $(KERN_DIR)/linker.ld -nostdlib

# ─── Source Files ───
C_SRCS      := $(KERN_DIR)/kmain.c \
               $(KERN_DIR)/vga.c \
               $(KERN_DIR)/interrupt.c \
               $(KERN_DIR)/memory.c \
               $(KERN_DIR)/timer.c \
               $(KERN_DIR)/keyboard.c \
               $(KERN_DIR)/serial.c \
               $(KERN_DIR)/process.c \
               $(KERN_DIR)/mexfs.c \
               $(KERN_DIR)/ai_engine.c \
               $(KERN_DIR)/shell.c \
               $(KERN_DIR)/gui.c \
               $(KERN_DIR)/window.c

# ─── Object Files ───
KERN_ASM_OBJS := $(OUT_DIR)/isr.o
C_OBJS      := $(patsubst %.c,$(OUT_DIR)/%.o,$(notdir $(C_SRCS)))
OBJS        := $(KERN_ASM_OBJS) $(C_OBJS)

# ─── Targets ───
.PHONY: all clean run iso debug format

all: $(OUT_DIR)/$(TARGET).iso

# ─── Create output directories ───
$(OUT_DIR):
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub

$(ISO_DIR):
	@mkdir -p $(ISO_DIR)/boot/grub

# ─── Boot sector (BINARY) ───
$(OUT_DIR)/boot.bin: $(BOOT_DIR)/boot.asm | $(OUT_DIR)
	@echo "  AS    $< (binary boot sector)"
	@$(AS) $(BOOT_ASFLAGS) $< -o $@

# ─── Stage 2 (BINARY) ───
$(OUT_DIR)/stage2.bin: $(BOOT_DIR)/stage2.asm | $(OUT_DIR)
	@echo "  AS    $< (binary stage2)"
	@$(AS) $(BOOT_ASFLAGS) $< -o $@

# ─── ISR (ELF64 for kernel linking) ───
$(OUT_DIR)/isr.o: $(KERN_DIR)/isr.asm | $(OUT_DIR)
	@echo "  AS    $<"
	@$(AS) $(KERN_ASFLAGS) $< -o $@

# ─── C compilation ───
$(OUT_DIR)/%.o: $(KERN_DIR)/%.c | $(OUT_DIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ─── Link kernel ───
$(OUT_DIR)/$(TARGET).bin: $(OBJS) $(KERN_DIR)/linker.ld | $(OUT_DIR)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)

# ─── Create floppy/disk image ───
$(OUT_DIR)/disk.img: $(OUT_DIR)/boot.bin $(OUT_DIR)/stage2.bin $(OUT_DIR)/$(TARGET).bin | $(OUT_DIR)
	@echo "  IMG   $@"
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	@dd if=$(OUT_DIR)/boot.bin of=$@ bs=512 conv=notrunc 2>/dev/null
	@dd if=$(OUT_DIR)/stage2.bin of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@dd if=$(OUT_DIR)/$(TARGET).bin of=$@ bs=512 seek=9 conv=notrunc 2>/dev/null

# ─── Create ISO with GRUB ───
$(OUT_DIR)/$(TARGET).iso: $(OUT_DIR)/$(TARGET).bin $(KERN_DIR)/grub.cfg | $(ISO_DIR)
	@echo "  ISO   $@"
	@cp $(OUT_DIR)/$(TARGET).bin $(ISO_DIR)/boot/$(TARGET).bin
	@cp $(KERN_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@$(GRUB_MKRESCUE) -o $@ $(ISO_DIR) 2>/dev/null || \
	 (echo "  Note: grub-mkrescue failed, trying grub2-mkrescue..." && \
	  grub2-mkrescue -o $@ $(ISO_DIR) 2>/dev/null) || \
	 (echo "  Note: Creating flat binary fallback..." && \
	  cp $(OUT_DIR)/$(TARGET).bin $@)

# ─── Run in QEMU ───
run: $(OUT_DIR)/disk.img
	@echo "  QEMU  MexaOS v$(VERSION) (Build $(BUILD))"
	@$(QEMU) -m 512M \
		-fda $(OUT_DIR)/disk.img \
		-boot a \
		-serial stdio \
		-vga std \
		-cpu qemu64 \
		-no-reboot \
		-no-shutdown

# ─── Debug mode ───
debug: $(OUT_DIR)/disk.img
	@echo "  DEBUG MexaOS v$(VERSION)"
	@$(QEMU) -m 512M \
		-fda $(OUT_DIR)/disk.img \
		-boot a \
		-serial stdio \
		-vga std \
		-cpu qemu64 \
		-s -S \
		-no-reboot \
		-no-shutdown

# ─── Clean ───
clean:
	@echo "  CLEAN build files"
	@rm -rf $(OUT_DIR) $(ISO_DIR)

# ─── Format code ───
format:
	@echo "  FORMAT source files"
	@find $(KERN_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i 2>/dev/null || \
	 echo "  Note: clang-format not available"

# ─── Help ───
help:
	@echo "MexaOS Build System v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build complete ISO image (default)"
	@echo "  run       - Build and run in QEMU (floppy with custom boot)"
	@echo "  debug     - Run with GDB debugging support"
	@echo "  clean     - Remove all build artifacts"
	@echo "  format    - Format source code"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  ARCH      = $(ARCH)"
	@echo "  VERSION   = $(VERSION)"
	@echo "  BUILD     = $(BUILD)"
