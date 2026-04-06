#include "serial.h"
#include <stdarg.h>

#define PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void serial_init(void) {
    outb(PORT + 1, 0x00);    // Disable interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int is_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

void serial_putc(char c) {
    while (is_transmit_empty() == 0);
    outb(PORT, c);
}

void serial_print(const char *s) {
    while (*s) serial_putc(*s++);
}

static void serial_print_hex(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    serial_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(hex[(val >> i) & 0xF]);
    }
}

void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1) != '\0') {
            fmt++;
            if (*fmt == 's') {
                serial_print(va_arg(args, const char *));
            } else if (*fmt == 'd') {
                int val = va_arg(args, int);
                if (val == 0) serial_putc('0');
                else {
                    char buf[10]; int i = 0;
                    while (val > 0) { buf[i++] = (val % 10) + '0'; val /= 10; }
                    while (i > 0) serial_putc(buf[--i]);
                }
            } else if (*fmt == 'x') {
                serial_print_hex(va_arg(args, uint32_t));
            }
        } else {
            serial_putc(*fmt);
        }
        fmt++;
    }
    va_end(args);
}
