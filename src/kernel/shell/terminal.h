#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t vga_color_t;

enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15,
};

#include "boot_info.h"

void terminal_init(boot_info_t *binfo);
void terminal_clear(void);
void terminal_setcolor(vga_color_t fg, vga_color_t bg);
void terminal_putchar(char c);
void terminal_write(const char *str);
void terminal_writeln(const char *str);
void terminal_printf(const char *fmt, ...);

void debug_ok(const char *msg);
void debug_warn(const char *msg);
void debug_err(const char *msg);
void debug_info(const char *msg);
void debug_hex(const char *label, uint32_t val);
void debug_dec(const char *label, uint32_t val);
void debug_separator(void);

#endif
