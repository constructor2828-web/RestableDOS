#include "mouse.h"
#include "serial.h"
#include <stdint.h>

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout-- && (inb(MOUSE_STATUS) & MOUSE_BBIT) == 0);
    } else {
        while (timeout-- && (inb(MOUSE_STATUS) & MOUSE_ABIT));
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    outb(MOUSE_STATUS, MOUSE_WRITE);
    mouse_wait(1);
    outb(MOUSE_PORT, a_write);
}

static uint8_t mouse_read_raw() {
    mouse_wait(0);
    return inb(MOUSE_PORT);
}

void mouse_init(void) {
    uint8_t status;

    // Enable the auxiliary mouse device
    mouse_wait(1);
    outb(MOUSE_STATUS, 0xA8);

    // Enable the interrupts
    mouse_wait(1);
    outb(MOUSE_STATUS, 0x20);
    mouse_wait(0);
    status = inb(MOUSE_PORT) | 2;
    mouse_wait(1);
    outb(MOUSE_STATUS, 0x60);
    mouse_wait(1);
    outb(MOUSE_PORT, status);

    // Use default settings
    mouse_write(0xF6);
    mouse_read_raw(); // Acknowledge

    // Enable data reporting
    mouse_write(0xF4);
    mouse_read_raw(); // Acknowledge
    
    serial_printf("[INFO] Mouse initialized.\n");
}

static int mouse_x = 0;
static int mouse_y = 0;

void mouse_read(mouse_state_t *state, int screen_w, int screen_h) {
    // Check if data is available
    if ((inb(MOUSE_STATUS) & 1) && (inb(MOUSE_STATUS) & 0x20)) {
        uint8_t status = inb(MOUSE_PORT);
        int8_t d_x = (int8_t)inb(MOUSE_PORT);
        int8_t d_y = (int8_t)inb(MOUSE_PORT);

        if (!(status & 0x80) && !(status & 0x40)) { // Valid packet
            mouse_x += d_x;
            mouse_y -= d_y; // PS/2 Y is inverted compared to screen coords

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= screen_w) mouse_x = screen_w - 1;
            if (mouse_y >= screen_h) mouse_y = screen_h - 1;

            state->buttons = status & 0x07;
        }
    }
    state->x = mouse_x;
    state->y = mouse_y;
}
