#include "disk.h"
#include "terminal.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outsw(uint16_t port, const void *addr, uint32_t word_count) {
    __asm__ volatile("rep outsw" : "+S"(addr), "+c"(word_count) : "d"(port));
}
static inline void insw(uint16_t port, void *addr, uint32_t word_count) {
    __asm__ volatile("rep insw" : "+D"(addr), "+c"(word_count) : "d"(port));
}

static void disk_wait(void) {
    for (int i = 0; i < 4; i++) inb(0x1F7);
}

void disk_write_sectors(uint32_t lba, uint8_t count, uint64_t buffer_addr) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    disk_wait();
    
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t) lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30); 

    uint16_t *ptr = (uint16_t*)buffer_addr;
    for (int i = 0; i < count; i++) {
        while (1) {
            uint8_t status = inb(0x1F7);
            if ((status & 0x80) == 0 && (status & 0x08) != 0) break;
            if (status & 0x01) {
                debug_err("DISK: Write error!");
                return;
            }
        }
        outsw(0x1F0, ptr, 256);
        ptr += 256;
    }

    outb(0x1F7, 0xE7);
    while (inb(0x1F7) & 0x80);
}

void disk_read_sectors(uint32_t lba, uint8_t count, uint64_t buffer_addr) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    disk_wait();
    
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t) lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);

    uint16_t *ptr = (uint16_t*)buffer_addr;
    for (int i = 0; i < count; i++) {
        while (1) {
            uint8_t status = inb(0x1F7);
            if ((status & 0x80) == 0 && (status & 0x08) != 0) break;
            if (status & 0x01) {
                debug_err("DISK: Read error!");
                return;
            }
        }
        insw(0x1F0, ptr, 256);
        ptr += 256;
    }
}
