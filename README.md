# RestableDOS v1.0 🚀

**RestableDOS** is a native x86_64 operating system, minimalist and efficiency-oriented, designed for modern hardware by complying with UEFI and Multiboot2 standards.

## Key Features 🌟

- **64-bit Architecture**: Native C kernel with a stable transition to Long Mode.
- **Interactive Aura Interface**: Minimalist graphical environment with procedural background, mouse support, and draggable system windows.
- **High Performance**: Implementing **Double Buffering** and fast 64-bit memory copies for a flicker-free experience.
- **Native Bootloader**: Uses **GRUB/Multiboot2**, eliminating the need for CSM (Legacy Boot).
- **Hardware Support**:
    - Active PCI device scanning.
    - Basic USB xHCI/EHCI support.
    - Native high-resolution Framebuffer handling.
    - PS/2 Mouse driver with real-time cursor interaction.
- **Root Filesystem (ROFS)**: Read-only filesystem mounted as a module.

## Project Structure 📁

- `src/boot/`: Multiboot2 loader and GRUB configuration.
- `src/kernel/`: System core, memory management, and drivers.
- `src/kernel/drivers/`: Hardware drivers (Serial, Mouse, PCI).
- `src/kernel/gui/`: Aura graphical interface and window manager.
- `src/kernel/shell/`: Interactive command terminal.
- `tools/`: Utilities for generating the ROFS disk image.
- `Makefile`: Automated build system.

## Build and Execution 🛠️

### Prerequisites
- `nasm`: x86 assembler.
- `gcc`: Compiler for ELF64.
- `grub-mkrescue`: ISO image generator.
- `xorriso`: Required by GRUB for CD-ROM images.
- `ovmf`: UEFI BIOS for QEMU.

### Commands
```bash
# Build the complete system (ISO)
make all

# Test in QEMU (UEFI Mode) - Runs with 1GB RAM and Double Buffering
make run

# Flash to real disk (/dev/sda)
make flash
```

## Credits ✒️

Developed with passion as an advanced OsDEV project under the name **RestableDOS**.

---
*© 2026 constructor2828-web. All rights reserved.*
