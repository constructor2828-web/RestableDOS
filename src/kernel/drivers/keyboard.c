#include "keyboard.h"
#include <stdint.h>

#define KB_DATA   0x60
#define KB_STATUS 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.'
};

static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?',
    0,  '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.'
};

static int shift_held = 0;

void keyboard_init(void) {
    // Initial state
    shift_held = 0;
}

char keyboard_get_char(void) {
    if (!(inb(KB_STATUS) & 1)) return 0; // Check if data available
    if (inb(KB_STATUS) & 0x20) return 0; // Ignore mouse data
    
    uint8_t sc = inb(KB_DATA);
    
    // Shift handling (0x2A left shift, 0x36 right shiftdown)
    if (sc == 0x2A || sc == 0x36) { shift_held = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift_held = 0; return 0; }
    
    if (sc & 0x80) return 0; // ignore key-up
    
    char c = shift_held ? scancode_shift[sc] : scancode_table[sc];
    return c;
}
