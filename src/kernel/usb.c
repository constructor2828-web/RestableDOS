#include "usb.h"
#include "shell/terminal.h"

static uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((((uint32_t)bus) << 16) | (((uint32_t)slot) << 11) |
                      (((uint32_t)func) << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    __asm__ volatile("outl %0, %1" : : "a"(address), "d"((uint16_t)0xCF8));
    uint32_t tmp;
    __asm__ volatile("inl %1, %0" : "=a"(tmp) : "d"((uint16_t)0xCFC));
    return tmp;
}

void usb_init(void) {
    debug_separator();
    debug_info("Scanning PCI bus for USB Host Controllers...");
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t reg0 = pci_config_read(bus, slot, func, 0);
                if ((reg0 & 0xFFFF) == 0xFFFF) continue; // No device

                uint32_t reg8 = pci_config_read(bus, slot, func, 8);
                uint8_t class     = (reg8 >> 24) & 0xFF;
                uint8_t subclass  = (reg8 >> 16) & 0xFF;
                uint8_t prog_if   = (reg8 >> 8) & 0xFF;

                if (class == 0x0C && subclass == 0x03) {
                    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
                    terminal_write("  [ USB ] Found ");
                    
                    if (prog_if == 0x00) terminal_write("UHCI");
                    else if (prog_if == 0x10) terminal_write("OHCI");
                    else if (prog_if == 0x20) terminal_write("EHCI");
                    else if (prog_if == 0x30) terminal_write("xHCI");
                    else terminal_write("Unknown");

                    terminal_printf(" Controller at PCI %d:%d.%d\n", bus, slot, func);
                    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
                }
            }
        }
    }
    
    debug_ok("USB Hardware Scan complete.");
}
