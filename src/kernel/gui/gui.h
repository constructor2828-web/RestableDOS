#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "boot_info.h" 

typedef struct {
    char title[32];
    int x, y, w, h;
    uint32_t bg_color;
    int is_visible;
    int is_dragging;
} window_t;

void gui_init(boot_info_t *binfo);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_text(const char *text, int x, int y, uint32_t color);
void gui_draw_window(window_t *win);
void gui_update(int mx, int my, int mb); // Call this in a loop
void gui_run(void); // Initial draw

#endif
