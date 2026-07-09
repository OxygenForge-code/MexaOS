/* ============================================================
 *  MexaOS Timer Driver
 * ============================================================ */

#ifndef MEXAOS_TIMER_H
#define MEXAOS_TIMER_H

#include <stdint.h>
#include <stddef.h>

/* ─── PIT Constants ─── */
#define PIT_FREQUENCY   1193182

/* ─── Functions ─── */
void timer_init(uint32_t freq);
uint64_t timer_get_ticks(void);
uint64_t timer_get_ms(void);
uint64_t timer_get_seconds(void);
uint64_t timer_get_uptime(void);

void timer_sleep_ms(uint64_t ms);
void timer_sleep_ticks(uint64_t ticks);

int timer_register_callback(void (*fn)(void), uint64_t interval_ms);
void timer_unregister_callback(int id);

void timer_format_uptime(char *buf, size_t size);
void timer_timestamp(char *buf, size_t size);

#endif
