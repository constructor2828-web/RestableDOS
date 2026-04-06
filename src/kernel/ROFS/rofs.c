#include "rofs.h"
#include "disk.h"
#include "terminal.h"

static uint64_t rofs_addr = 0;
#define ROFS_ADDR ((rofs_header_t *)rofs_addr)

#pragma pack(push, 1)
typedef struct {
    char name[32];
    uint32_t size;
    uint32_t offset;
    uint32_t type;
} rofs_entry_t;

typedef struct {
    char magic[4];
    uint32_t num_entries;
    rofs_entry_t entries[];
} rofs_header_t;
#pragma pack(pop)

static int kstrcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void kstrcpy(char *dst, const char *src, int max) {
    int i = 0;
    for (; i + 1 < max && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}

void rofs_init(uint64_t addr) {
    rofs_addr = addr;
    rofs_header_t *hdr = ROFS_ADDR;
    if (hdr->magic[0] == 'R' && hdr->magic[1] == 'O' && hdr->magic[2] == 'F' && hdr->magic[3] == 'S') {
        debug_ok("ROFS mounted via UEFI Loader");
        debug_dec("ROFS Entries", hdr->num_entries);
    } else {
        debug_err("No Valid ROFS volume found in RAM");
    }
}

void rofs_ls(void) {
    rofs_header_t *hdr = ROFS_ADDR;
    if (hdr->magic[0] != 'R') {
        debug_err("ls: ROFS not mounted");
        return;
    }
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writeln("\n  ls /rofs");
    debug_separator();
    if (hdr->num_entries == 0) {
        terminal_setcolor(VGA_DARK_GREY, VGA_BLACK);
        terminal_writeln("  (empty)");
    }
    for (uint32_t i = 0; i < hdr->num_entries; i++) {
        terminal_setcolor(VGA_WHITE, VGA_BLACK);
        terminal_printf("  [FILE] %-24s  size=%u\n", hdr->entries[i].name, hdr->entries[i].size);
    }
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_putchar('\n');
}

int rofs_read_file(const char *name, uint8_t *buf, uint32_t max_len) {
    rofs_header_t *hdr = ROFS_ADDR;
    if (hdr->magic[0] != 'R') return -1;
    for (uint32_t i = 0; i < hdr->num_entries; i++) {
        if (kstrcmp(name, hdr->entries[i].name) == 0) {
            uint32_t sz = hdr->entries[i].size;
            if (sz > max_len) sz = max_len;
            uint8_t *src = (uint8_t *)(uintptr_t)(rofs_addr + hdr->entries[i].offset);
            for (uint32_t j = 0; j < sz; j++) buf[j] = src[j];
            return sz;
        }
    }
    return -1;
}

int rofs_write_file(const char *name, const uint8_t *data, uint32_t len) {
    rofs_header_t *hdr = ROFS_ADDR;
    if (hdr->magic[0] != 'R') return -1;
    
    for (uint32_t i = 0; i < hdr->num_entries; i++) {
        if (kstrcmp(name, hdr->entries[i].name) == 0) {
            debug_err("File exists (Overwrite not supported)");
            return -1;
        }
    }

    if (hdr->num_entries >= 256) {
        debug_err("No directory space left");
        return -1;
    }
    
    uint32_t i = hdr->num_entries;
    kstrcpy(hdr->entries[i].name, name, 31);
    hdr->entries[i].size = len;
    hdr->entries[i].type = 0;
    
    uint32_t next_off = 8 + 256 * sizeof(rofs_entry_t);
    if (i > 0) {
        next_off = hdr->entries[i-1].offset + hdr->entries[i-1].size;
    }
    hdr->entries[i].offset = next_off;
    
    if (next_off + len > 32768) {
        debug_err("Out of storage space!");
        return -1;
    }
    
    uint8_t *dst = (uint8_t *)(uintptr_t)(rofs_addr + next_off);
    for (uint32_t j = 0; j < len; j++) dst[j] = data[j];
    
    hdr->num_entries++;
    
    terminal_write("  [ INFO ] Syncing disk... ");
    disk_write_sectors(65, 64, rofs_addr);
    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writeln("Committed!");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    
    return 0;
}
