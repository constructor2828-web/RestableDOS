#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_get_char(void); // Non-blocking: returns 0 if no key

#endif
