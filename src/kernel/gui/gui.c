#include "gui.h"
#include "shell/font8x8.h"
#include <stddef.h>

static boot_info_t *fb_info;
static int old_mouse_x = 0;
static int old_mouse_y = 0;

// Double Buffering: 1024x768 is ~3MB.
static uint32_t backbuffer[1024 * 768]; 
// Background Cache: Store the pre-rendered gradient
static uint32_t bg_buffer[1024 * 768];

typedef struct {
    char title[32];
    int x, y, w, h;
    int is_dragging;
} gui_win_t;

static gui_win_t windows[4] = {
    {"Terminal", 40, 60, 480, 320, 0},
    {"System Monitor", 550, 40, 280, 220, 0},
    {"Clock Settings", 550, 280, 200, 100, 0},
    {"About RestableDOS", 100, 400, 350, 120, 0}
};

static void gui_draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= fb_info->screen_width || y < 0 || y >= fb_info->screen_height) return;
    backbuffer[y * fb_info->screen_width + x] = color;
}

// Optimized 64-bit memory copy
static void fast_memcpy64(void *dst, const void *src, uint32_t n_pixels) {
    uint64_t *d64 = (uint64_t *)dst;
    const uint64_t *s64 = (const uint64_t *)src;
    uint32_t count = n_pixels / 2;
    for (uint32_t i = 0; i < count; i++) {
        d64[i] = s64[i];
    }
}

static void gui_flip(void) {
    uint32_t *dest = (uint32_t *)fb_info->framebuffer_base;
    fast_memcpy64(dest, backbuffer, fb_info->screen_width * fb_info->screen_height);
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
    gui_draw_rect(x + w - 22, y + 6, 14, 14, 0x00441111); // Close
    gui_draw_text("x", x + w - 18, y + 9, 0x00FFFFFF);

    if (win->x == 40) { // Terminal
        gui_draw_text("RestableDOS (x86_64) - TTY0", x + 15, y + 40, 0x0000FF00);
        gui_draw_text("root@restabledos:~$ help", x + 15, y + 60, 0x00BBBBBB);
        gui_draw_text("ls  cat  write  pci  gfx", x + 15, y + 80, 0x00888888);
        gui_draw_text("_", x + 15, y + 100, 0x00FFFFFF);
    } else if (win->x == 550 && win->y < 200) { // Monitor
        static uint32_t tick = 0; tick++;
        gui_draw_text("CPU Usage:  Active", x + 15, y + 40, 0x0000AAFF);
        gui_draw_rect(x + 15, y + 55, 180, 6, 0x00222222);
        gui_draw_rect(x + 15, y + 55, 10 + (tick % 150), 6, 0x0000AAFF);
        gui_draw_text("MEM Usage: 1.4 MB", x + 15, y + 75, 0x0000FF00);
    } else if (win->x == 100 && win->y >= 400) { // About Window
        gui_draw_text("Stage2.bin [OSDV],  - 20:53", x + 15, y + 40, 0x00FFFFFF);
        gui_draw_text("8282rotcurtsnoC [OSDV],  - 20:53", x + 15, y + 65, 0x00AAAAAA);
        gui_draw_text("(c) 2026 constructor2828-web & Kernelist", x + 15, y + 90, 0x00888888);
    }
}

// Pre-render the background into bg_buffer once
static void gui_cache_background(void) {
    for (uint32_t y = 0; y < fb_info->screen_height; y++) {
        uint8_t r = 0, g = 5, b = (uint8_t)(10 + (y * 40 / fb_info->screen_height));
        uint32_t color = (r << 16) | (g << 8) | b;
        for (uint32_t x = 0; x < fb_info->screen_width; x++) {
            bg_buffer[y * fb_info->screen_width + x] = color;
        }
    }
}

void gui_init(boot_info_t *binfo) {
    fb_info = binfo;
    gui_cache_background();
}

static void draw_cursor(int x, int y) {
    for(int i=0; i<8; i++) {
        for(int j=0; j<i; j++) {
            gui_draw_pixel(x + j, y + i, 0x00FFFFFF);
        }
    }
}

void gui_update(int mx, int my, int mb) {
    static int last_mb = 0;
    int dx = mx - old_mouse_x;
    int dy = my - old_mouse_y;

    for (int i = 0; i < 4; i++) {
        if (mb == 1 && !last_mb) {
            if (mx > windows[i].x && mx < windows[i].x + windows[i].w &&
                my > windows[i].y && my < windows[i].y + 25) {
                windows[i].is_dragging = 1;
            }
        }
        if (mb == 0) windows[i].is_dragging = 0;
        if (windows[i].is_dragging) { windows[i].x += dx; windows[i].y += dy; }
    }

    // 1. Fast Background Copy (Replacing the slow pixel-by-pixel loop)
    fast_memcpy64(backbuffer, bg_buffer, fb_info->screen_width * fb_info->screen_height);

    // 2. Specialized Redraws
    // Taskbar (Static for now, but needs to be in backbuffer)
    gui_draw_rect(0, fb_info->screen_height - 35, fb_info->screen_width, 35, 0x00111111);
    gui_draw_text("Restable", 15, fb_info->screen_height - 23, 0x0000AAFF);
    gui_draw_text("18:52:10", fb_info->screen_width - 80, fb_info->screen_height - 23, 0x00FFFFFF);

    for (int i = 0; i < 4; i++) gui_draw_window_internal(&windows[i]);
    draw_cursor(mx, my);
    
    // 3. FLIP
    gui_flip();

    old_mouse_x = mx;
    old_mouse_y = my;
    last_mb = mb;
}

void gui_run(void) {
    gui_update(0, 0, 0);
}
