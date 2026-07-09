/* ============================================================
 *  MexaOS Serial Port Header
 * ============================================================ */

#ifndef SERIAL_H
#define SERIAL_H

#include "../../include/mexaos.h"

void serial_init(uint16_t port);
void serial_putc(uint16_t port, char c);
void serial_puts(uint16_t port, const char *s);
char serial_getc(uint16_t port);
void serial_put_hex(uint16_t port, uint64_t val);
void serial_put_dec(uint16_t port, uint64_t val);
int serial_is_available(void);

#endif /* SERIAL_H */
