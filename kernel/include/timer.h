/* ============================================================
 *  MexaOS Timer Header
 * ============================================================ */

#ifndef TIMER_H
#define TIMER_H

#include "../../include/mexaos.h"

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

/* Forward declaration for scheduler */
void scheduler_tick(void);

#endif /* TIMER_H */
