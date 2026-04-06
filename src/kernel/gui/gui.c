#include "gui.h"
#include "shell/font8x8.h"
#include <stddef.h>

static boot_info_t *fb_info;
static int old_mouse_x = 0;
static int old_mouse_y = 0;

typedef struct {
    char title[32];
    int x, y, w, h;
    int is_dragging;
} gui_win_t;

static gui_win_t windows[3] = {
    {"Terminal", 40, 60, 480, 320, 0},
    {"System Monitor", 550, 40, 280, 220, 0},
    {"Clock Settings", 550, 280, 200, 100, 0}
};

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
                if (bitmap[i] & (0x80 >> j)) {
                    gui_draw_pixel(x + j, y + i, color);
                }
            }
        }
        x += 8;
        text++;
    }
}

static void gui_draw_window_internal(gui_win_t *win) {
    int x = win->x; int y = win->y; int w = win->w; int h = win->h;
    gui_draw_rect(x + 4, y + 4, w, h, 0x000A0A0A); 
    gui_draw_rect(x, y, w, h, 0x002D2D2D); 
    gui_draw_rect(x + 1, y + 1, w - 2, h - 2, 0x003F3F3F); 
    gui_draw_rect(x + 2, y + 25, w - 4, h - 27, 0x001A1A1A); 
    gui_draw_rect(x + 2, y + 2, w - 4, 22, 0x002A2A2A); 
    gui_draw_text(win->title, x + 10, y + 9, 0x00E0E0E0);
    gui_draw_rect(x + w - 22, y + 6, 14, 14, 0x00441111); // Close button
    gui_draw_text("x", x + w - 18, y + 9, 0x00FFFFFF);
}

static void gui_draw_background(void) {
    for (uint32_t y = 0; y < fb_info->screen_height; y++) {
        uint8_t r = 0, g = 5, b = (uint8_t)(10 + (y * 40 / fb_info->screen_height));
        uint32_t color = (r << 16) | (g << 8) | b;
        for (uint32_t x = 0; x < fb_info->screen_width; x++) {
            gui_draw_pixel(x, y, color);
        }
    }
    // Taskbar
    gui_draw_rect(0, fb_info->screen_height - 35, fb_info->screen_width, 35, 0x00111111);
    gui_draw_rect(4, fb_info->screen_height - 31, 100, 26, 0x00222222);
    gui_draw_text("Restable", 15, fb_info->screen_height - 23, 0x0000AAFF);
}

void gui_init(boot_info_t *binfo) {
    fb_info = binfo;
}

static void draw_cursor(int x, int y, int erase) {
    uint32_t color = erase ? 0x00000000 : 0x00FFFFFF; // This is naive, should restore BG
    // Simple 5x5 cursor
    for(int i=0; i<5; i++) {
        for(int j=0; j<i+1; j++) {
            gui_draw_pixel(x + j, y + i, color);
        }
    }
}

void gui_update(int mx, int my, int mb) {
    static int last_mb = 0;
    int dx = mx - old_mouse_x;
    int dy = my - old_mouse_y;

    // 1. Interaction Logic
    for (int i = 0; i < 3; i++) {
        // Dragging check (Title bar area)
        if (mb == 1 && !last_mb) {
            if (mx > windows[i].x && mx < windows[i].x + windows[i].w &&
                my > windows[i].y && my < windows[i].y + 25) {
                windows[i].is_dragging = 1;
            }
        }
        if (mb == 0) windows[i].is_dragging = 0;

        if (windows[i].is_dragging) {
            windows[i].x += dx;
            windows[i].y += dy;
        }
    }

    // 2. Simple Redraw (Full redraw is slow, but keeps it simple for now)
    // To minimize flicker, only redraw if mouse moved or dragging
    if (dx != 0 || dy != 0 || mb != last_mb) {
        gui_draw_background();
        for (int i = 0; i < 3; i++) gui_draw_window_internal(&windows[i]);
        draw_cursor(mx, my, 0);
    }

    old_mouse_x = mx;
    old_mouse_y = my;
    last_mb = mb;
}

void gui_run(void) {
    gui_draw_background();
    for (int i = 0; i < 3; i++) gui_draw_window_internal(&windows[i]);
    draw_cursor(0, 0, 0);
}
