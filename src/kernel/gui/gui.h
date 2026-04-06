#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "boot_info.h"

void gui_init(boot_info_t *binfo);
void gui_run(void);

void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_window(const char *title, int x, int y, int w, int h);
void gui_draw_text(const char *text, int x, int y, uint32_t color);

#endif
