/* ============================================================
 *  MexaOS Serial Port Driver
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>

#include "../../include/mexaos.h"
#include "include/serial.h"

static uint8_t serial_available = 0;
static uint16_t serial_port = SERIAL_COM1;

void serial_init(uint16_t port) {
    serial_port = port;
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    outb(port + 0, 0x03);
    outb(port + 1, 0x00);
    outb(port + 3, 0x03);
    outb(port + 2, 0xC7);
    outb(port + 4, 0x0B);
    outb(port + 4, 0x1E);
    outb(port + 0, 0xAE);
    
    if (inb(port + 0) != 0xAE) { serial_available = 0; return; }
    
    outb(port + 4, 0x0F);
    serial_available = 1;
}

void serial_putchar(char c) {
    if (!serial_available) return;
    while ((inb(serial_port + 5) & 0x20) == 0);
    outb(serial_port, c);
}

void serial_puts(const char *s) {
    while (*s) {
        serial_putchar(*s++);
    }
}
