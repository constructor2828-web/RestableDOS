#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include "boot_info.h" 

typedef struct {
    int x, y;
    uint8_t buttons;
} mouse_state_t;

void mouse_init(void);
void mouse_read(mouse_state_t *state, int screen_w, int screen_h);

#endif
