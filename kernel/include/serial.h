/* ============================================================
 *  MexaOS Serial Port Driver
 * ============================================================ */

#ifndef MEXAOS_SERIAL_H
#define MEXAOS_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8

void serial_init(uint16_t port);
void serial_putchar(char c);
void serial_puts(const char *s);

#endif
