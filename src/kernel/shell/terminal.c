#include "terminal.h"
#include "font8x8.h"
#include <stdarg.h>

static boot_info_t *boot_info;
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t fg_color = 0xFFFFFFFF; // White
static uint32_t bg_color = 0x00000000; // Black

/* Standard VGA 16-color palette to 32-bit BGR/RGB mapping */
static uint32_t color_table[] = {
    0x00000000, // Black
    0x00AA0000, // Blue
    0x0000AA00, // Green
    0x00AAAA00, // Cyan
    0x000000AA, // Red
    0x00AA00AA, // Magenta
    0x000055AA, // Brown
    0x00AAAAAA, // Light Grey
    0x00555555, // Dark Grey
    0x00FF5555, // Light Blue
    0x0055FF55, // Light Green
    0x0055FFFF, // Light Cyan
    0x00FF5555, // Light Red
    0x00FF55FF, // Light Magenta
    0x00FFFF55, // Light Yellow
    0x00FFFFFF  // White
};

void draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= boot_info->screen_width || y >= boot_info->screen_height) return;
    uint32_t *fb = (uint32_t *)boot_info->framebuffer_base;
    fb[y * boot_info->pixels_per_scanline + x] = color;
}

void draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    uint8_t *bitmap = font8x8_basic[(uint8_t)c];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (bitmap[i] & (1 << j)) {
                draw_pixel(x + j, y + i, fg);
            } else {
                draw_pixel(x + j, y + i, bg);
            }
        }
    }
}

void terminal_init(boot_info_t *binfo) {
    boot_info = binfo;
    terminal_clear();
}

void terminal_clear(void) {
    uint32_t *fb = (uint32_t *)boot_info->framebuffer_base;
    for (uint32_t i = 0; i < boot_info->screen_width * boot_info->screen_height; i++) {
        fb[i] = bg_color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void terminal_setcolor(vga_color_t fg, vga_color_t bg) {
    if (fg < 16) fg_color = color_table[fg];
    if (bg < 16) bg_color = color_table[bg];
}

void terminal_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 12; // 8px font + 4px spacing
    } else {
        draw_char(c, cursor_x, cursor_y, fg_color, bg_color);
        cursor_x += 8;
        if (cursor_x + 8 > boot_info->screen_width) {
            cursor_x = 0;
            cursor_y += 12;
        }
    }
    
    // Simple scrolling: if we hit bottom, reset to top (could be improved)
    if (cursor_y + 12 > boot_info->screen_height) {
        terminal_clear();
    }
}

void terminal_write(const char *str) {
    while (*str) terminal_putchar(*str++);
}

void terminal_writeln(const char *str) {
    terminal_write(str);
    terminal_putchar('\n');
}

/* ── Minimal printf implementation ────────────────────────────────────────── */
static void print_uint(uint32_t n, int base) {
    char buf[32];
    int i = 0;
    if (n == 0) { terminal_putchar('0'); return; }
    while (n > 0) {
        int rem = n % base;
        buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A');
        n /= base;
    }
    while (--i >= 0) terminal_putchar(buf[i]);
}

void terminal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            if (*fmt == 's') terminal_write(va_arg(args, char*));
            else if (*fmt == 'd') print_uint(va_arg(args, uint32_t), 10);
            else if (*fmt == 'x') print_uint(va_arg(args, uint32_t), 16);
            else if (*fmt == 'c') terminal_putchar((char)va_arg(args, int));
        } else {
            terminal_putchar(*fmt);
        }
        fmt++;
    }
    va_end(args);
}

/* ── Debug helpers ─────────────────────────────────────────────────────────── */
void debug_ok(const char *msg) {
    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_write("  [ OK ] ");
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln(msg);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

void debug_info(const char *msg) {
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_write("  [ INFO ] ");
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln(msg);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

void debug_err(const char *msg) {
    terminal_setcolor(VGA_LIGHT_RED, VGA_BLACK);
    terminal_write("  [ ERR ] ");
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln(msg);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

void debug_warn(const char *msg) {
    terminal_setcolor(VGA_LIGHT_BROWN, VGA_BLACK);
    terminal_write("  [ WARN ] ");
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln(msg);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

void debug_hex(const char *label, uint32_t val) {
    terminal_write("  [ ");
    terminal_write(label);
    terminal_write(" ]: 0x");
    print_uint(val, 16);
    terminal_putchar('\n');
}

void debug_dec(const char *label, uint32_t val) {
    terminal_write("  [ ");
    terminal_write(label);
    terminal_write(" ]: ");
    print_uint(val, 10);
    terminal_putchar('\n');
}

void debug_separator(void) {
    terminal_setcolor(VGA_DARK_GREY, VGA_BLACK);
    terminal_writeln("------------------------------------------------------------------------");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}