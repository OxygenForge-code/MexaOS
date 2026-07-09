/* ============================================================
 *  MexaOS Serial Port Driver
 *  COM1 for kernel debugging and logging
 * ============================================================ */

#include "include/serial.h"

#define SERIAL_DATA(port)       (port)
#define SERIAL_FIFO(port)       ((port) + 2)
#define SERIAL_LINE(port)       ((port) + 3)
#define SERIAL_MODEM(port)      ((port) + 4)
#define SERIAL_LINE_STATUS(port) ((port) + 5)

#define SERIAL_DATA_READY       0x01
#define SERIAL_EMPTY_TR         0x20
#define SERIAL_EMPTY_DH         0x40

static int serial_available = 0;

void serial_init(uint16_t port) {
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    outb(port + 0, 0x01);
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

static int serial_tx_empty(uint16_t port) {
    return inb(SERIAL_LINE_STATUS(port)) & SERIAL_EMPTY_TR;
}

static int serial_rx_ready(uint16_t port) {
    return inb(SERIAL_LINE_STATUS(port)) & SERIAL_DATA_READY;
}

void serial_putc(uint16_t port, char c) {
    if (!serial_available) return;
    while (!serial_tx_empty(port));
    outb(SERIAL_DATA(port), c);
}

void serial_puts(uint16_t port, const char *s) {
    if (!serial_available) return;
    while (*s) serial_putc(port, *s++);
}

char serial_getc(uint16_t port) {
    if (!serial_available) return 0;
    if (serial_rx_ready(port)) return inb(SERIAL_DATA(port));
    return 0;
}

void serial_put_hex(uint16_t port, uint64_t val) {
    const char *hex = "0123456789ABCDEF";
    serial_puts(port, "0x");
    int shift = 60, started = 0;
    while (shift >= 0) {
        int digit = (val >> shift) & 0xF;
        if (digit || started || shift == 0) {
            serial_putc(port, hex[digit]);
            started = 1;
        }
        shift -= 4;
    }
}

void serial_put_dec(uint16_t port, uint64_t val) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    if (val == 0) { serial_putc(port, '0'); return; }
    while (val > 0) { buf[i--] = '0' + (val % 10); val /= 10; }
    serial_puts(port, &buf[i + 1]);
}

int serial_is_available(void) { return serial_available; }
