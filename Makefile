CC      = gcc
LD      = ld
AS      = nasm
OBJCOPY = objcopy
QEMU    = qemu-system-x86_64
BUILD   = ./build
ISO     = $(BUILD)/RenamedOS.iso

# C Flags - Native 64-bit ELF
KERNEL_CFLAGS = -m64 -ffreestanding -fno-pie -nostdlib -fno-builtin \
                -Isrc/kernel -Isrc/kernel/ROFS -Isrc/kernel/shell -Isrc/kernel/drivers \
                -mcmodel=large -mno-red-zone

# Linker Flags - ELF64
KERNEL_LDFLAGS = -m elf_x86_64 -T linker.ld

# Sources
SRC_KERNEL = $(shell find src/kernel -name "*.c")
OBJ_KERNEL = $(patsubst src/kernel/%.c, $(BUILD)/kernel/%.o, $(SRC_KERNEL))
OBJ_ASM    = $(BUILD)/boot/multiboot_header.o $(BUILD)/kernel/entry.o

.PHONY: all clean run build_iso flash

all: prebuild $(ISO)

$(ISO): $(BUILD)/kernel.bin $(BUILD)/rofs.bin src/boot/grub.cfg
	@echo "[ISO]  Packaging with grub-mkrescue..."
	@mkdir -p $(BUILD)/isodir/boot/grub
	@cp $(BUILD)/kernel.bin $(BUILD)/isodir/boot/kernel.bin
	@cp $(BUILD)/rofs.bin $(BUILD)/isodir/boot/rofs.bin
	@cp src/boot/grub.cfg $(BUILD)/isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) $(BUILD)/isodir
	@echo "[ISO]  Done -> $(ISO)"

$(BUILD)/kernel.bin: $(OBJ_ASM) $(OBJ_KERNEL)
	@echo "[LD]   Linking Kernel ELF..."
	@$(LD) $(KERNEL_LDFLAGS) -o $@ $^

$(BUILD)/boot/multiboot_header.o: src/boot/multiboot_header.asm
	@echo "[AS]   Multiboot Header -> $<"
	@mkdir -p $(dir $@)
	@$(AS) -f elf64 $< -o $@

$(BUILD)/kernel/entry.o: src/kernel/entry.asm
	@echo "[AS]   Kernel Entry -> $<"
	@mkdir -p $(dir $@)
	@$(AS) -f elf64 $< -o $@

$(BUILD)/kernel/%.o: src/kernel/%.c
	@echo "[CC]   Kernel -> $<"
	@mkdir -p $(dir $@)
	@$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD)/rofs.bin: tools/mkrofs.c
	@echo "[HOST] Compiling mkrofs..."
	@gcc -O2 -o $(BUILD)/mkrofs tools/mkrofs.c
	@echo "[ROFS] Packing root filesystem..."
	@$(BUILD)/mkrofs rofs_root $@

prebuild:
	@mkdir -p $(BUILD)

run: all
	@echo "[RUN]  Launching QEMU (UEFI)..."
	$(QEMU) -bios /usr/share/ovmf/OVMF.fd -cdrom $(ISO) -m 1G -net none -serial stdio

flash: all
	@echo "=========================================================="
	@echo "  WARNING: This will DESTROY all data on /dev/sda"
	@echo "  Target: /dev/sda"
	@echo "=========================================================="
	@echo -n "Starting flash in 3 seconds... "
	@sleep 3
	@echo 101010 | sudo -S dd if=$(ISO) of=/dev/sda bs=4M status=progress
	@echo 101010 | sudo -S sync
	@echo "Flash complete. You can now reboot the machine."

clean:
	rm -rf $(BUILD)