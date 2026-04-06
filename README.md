# RestableDOS v1.0 🚀

**RestableDOS** es un sistema operativo nativo x86_64, minimalista y orientado a la eficiencia, diseñado para hardware moderno cumpliendo con los estándares UEFI y Multiboot2.

## Características Principales 🌟

- **Arquitectura de 64 bits**: Kernel escrito en C nativo con una transición estable a Modo Largo.
- **Interfaz Aura**: Entorno gráfico minimalista con fondo procedimental y ventanas de sistema.
- **Bootloader Nativo**: Utiliza **GRUB/Multiboot2** eliminando la necesidad de CSM (Legacy Boot).
- **Subprocesos y Hardware**:
    - Escaneo activo de dispositivos PCI.
    - Soporte básico de USB xHCI/EHCI.
    - Manejo nativo de Framebuffer de alta resolución.
- **Root Filesystem (ROFS)**: Sistema de archivos de solo lectura montado como módulo.

## Estructura del Proyecto 📁

- `src/boot/`: Cargador Multiboot2 y configuración de GRUB.
- `src/kernel/`: Núcleo del sistema, gestión de memoria y controladores.
- `src/kernel/gui/`: Interfaz gráfica Aura.
- `src/kernel/shell/`: Terminal de comandos interactivo (20+ comandos).
- `tools/`: Utilidades para generar la imagen de disco ROFS.
- `Makefile`: Sistema de construcción automatizado.

## Compilación y Ejecución 🛠️

### Requisitos
- `nasm`: Ensamblador x86.
- `gcc`: Compilador para ELF64.
- `grub-mkrescue`: Generador de imágenes ISO.
- `xorriso`: Requerido por GRUB para CD-ROM.
- `ovmf`: Bios UEFI para QEMU.

### Comandos
```bash
# Construir el sistema completo (ISO)
make all

# Probar en QEMU (Modo UEFI CDI)
make run

# Flashear a disco real (/dev/sda)
make flash
```

## Créditos ✒️

Desarrollado con pasión como proyecto de OsDEV avanzado bajo el nombre de **RestableDOS**.

---
*© 2026 constructor2828-web. Todos los derechos reservados.*
