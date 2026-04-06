#include "gui.h"
#include "shell/font8x8.h"
#include <stddef.h>

static boot_info_t *fb_info;

static void gui_draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= fb_info->screen_width || y < 0 || y >= fb_info->screen_height) return;
    uint32_t *fb = (uint32_t *)fb_info->framebuffer_base;
    fb[y * fb_info->pixels_per_scanline + x] = color;
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            gui_draw_pixel(x + j, y + i, color);
        }
    }
}

void gui_draw_text(const char *text, int x, int y, uint32_t color) {
    while (*text) {
        uint8_t *bitmap = font8x8_basic[(uint8_t)*text];
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (bitmap[i] & (1 << j)) {
                    gui_draw_pixel(x + j, y + i, color);
                }
            }
        }
        x += 8;
        text++;
    }
}

static void gui_draw_background(void) {
    // 1. Deep Blue Gradient
    for (uint32_t y = 0; y < fb_info->screen_height; y++) {
        uint8_t r = 0, g = 5, b = (uint8_t)(10 + (y * 40 / fb_info->screen_height));
        uint32_t color = (r << 16) | (g << 8) | b;
        for (uint32_t x = 0; x < fb_info->screen_width; x++) {
            gui_draw_pixel(x, y, color);
        }
    }

    // 2. Subtle Grid (every 40px)
    for (uint32_t y = 0; y < fb_info->screen_height; y += 40) {
        for (uint32_t x = 0; x < fb_info->screen_width; x++) {
            if (x % 40 == 0) continue; 
            uint32_t *fb = (uint32_t *)fb_info->framebuffer_base;
            uint32_t c = fb[y * fb_info->pixels_per_scanline + x];
            // Lighten the grid lines slightly
            gui_draw_pixel(x, y, c + 0x010101);
        }
    }
    for (uint32_t x = 0; x < fb_info->screen_width; x += 40) {
        for (uint32_t y = 0; y < fb_info->screen_height; y++) {
            uint32_t *fb = (uint32_t *)fb_info->framebuffer_base;
            uint32_t c = fb[y * fb_info->pixels_per_scanline + x];
            gui_draw_pixel(x, y, c + 0x010101);
        }
    }

    // 3. Floating "Code" Snippets (simple horizontal gray bars)
    for (int i = 0; i < 20; i++) {
        int lx = (i * 12345) % (fb_info->screen_width - 150);
        int ly = (i * 67890) % (fb_info->screen_height - 100);
        int lw = 30 + (i * 7) % 80;
        gui_draw_rect(lx, ly, lw, 2, 0x00222222);
    }
}

void gui_draw_window(const char *title, int x, int y, int w, int h) {
    gui_draw_rect(x + 4, y + 4, w, h, 0x000A0A0A); // Shadow
    gui_draw_rect(x, y, w, h, 0x002D2D2D); // Outer border (Dark Gray)
    gui_draw_rect(x + 1, y + 1, w - 2, h - 2, 0x003F3F3F); // Inner border
    gui_draw_rect(x + 2, y + 25, w - 4, h - 27, 0x001A1A1A); // Content area (Very Dark)
    
    // Modern Title bar
    gui_draw_rect(x + 2, y + 2, w - 4, 22, 0x002A2A2A); 
    gui_draw_text(title, x + 10, y + 9, 0x00E0E0E0);
    
    // Minimalist Close button
    gui_draw_rect(x + w - 22, y + 6, 14, 14, 0x00441111);
    gui_draw_text("x", x + w - 18, y + 9, 0x00FFFFFF);
}

void gui_init(boot_info_t *binfo) {
    fb_info = binfo;
}

void gui_run(void) {
    gui_draw_background();
    
    // Bottom Taskbar (Glassmorphic look)
    gui_draw_rect(0, fb_info->screen_height - 35, fb_info->screen_width, 35, 0x00111111);
    gui_draw_rect(0, fb_info->screen_height - 36, fb_info->screen_width, 1, 0x00333333); // Top line
    
    // Start Button
    gui_draw_rect(4, fb_info->screen_height - 31, 100, 26, 0x00222222);
    gui_draw_text("Restable", 15, fb_info->screen_height - 23, 0x0000AAFF);
    
    // App 1: Terminal
    gui_draw_window("RestableDOS Terminal v1.0", 40, 60, 480, 320);
    gui_draw_text("RestableDOS (x86_64) - TTY0", 55, 95, 0x0000FF00);
    gui_draw_text("root@restabledos:~$ help", 55, 115, 0x00BBBBBB);
    gui_draw_text("Commands: help, ls, cat, write, pci...", 55, 135, 0x00888888);
    gui_draw_text("_", 55, 155, 0x00FFFFFF);
    
    // App 2: Status Board (Task Manager)
    gui_draw_window("System Monitor", 550, 40, 280, 220);
    gui_draw_text("CPU Usage:  2%", 565, 75, 0x0000AAFF);
    gui_draw_rect(565, 90, 250, 6, 0x00222222);
    gui_draw_rect(565, 90, 10, 6, 0x0000AAFF);
    
    gui_draw_text("MEM Usage: 1.4MB", 565, 115, 0x0000FF00);
    gui_draw_rect(565, 130, 250, 6, 0x00222222);
    gui_draw_rect(565, 130, 40, 6, 0x0000FF00);
    
    gui_draw_text("Threads Act: 12", 565, 155, 0x00BBBBBB);
    gui_draw_text("Uptime: 00:01:42", 565, 185, 0x00888888);

    // App 3: Clock (Taskbar Right)
    gui_draw_text("18:44:30", fb_info->screen_width - 80, fb_info->screen_height - 23, 0x00FFFFFF);
}
