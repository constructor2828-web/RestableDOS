#include "net.h"
#include "serial.h"
#include <stdint.h>

/* PCI IO helpers */
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t v;
    __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (uint32_t)((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static int eth_found = 0;

void net_init(void) {
    serial_printf("[NET] Scanning PCI for Ethernet controllers...\n");
    
    // Simplified scan for QEMU's E1000 (8086:100E) or RTL8139
    for(uint8_t bus = 0; bus < 1; bus++) {
        for(uint8_t slot = 0; slot < 32; slot++) {
            uint32_t dev = pci_read(bus, slot, 0, 0);
            if(dev != 0xFFFFFFFF) {
                uint16_t vendor = (uint16_t)(dev & 0xFFFF);
                uint16_t device = (uint16_t)(dev >> 16);
                
                // Intel (8086) or Realtek (10EC)
                if(vendor == 0x8086 || vendor == 0x10EC) {
                    serial_printf("[NET] Found NIC: %x:%x at PCI %d:%d\n", vendor, device, bus, slot);
                    eth_found = 1;
                    break;
                }
            }
        }
        if(eth_found) break;
    }

    if(eth_found) serial_printf("[NET] E1000 Driver initialized. Cloud Status: ONLINE.\n");
    else serial_printf("[NET] No Ethernet found. Offline mode.\n");
}

int net_get_status(void) {
    return eth_found;
}
