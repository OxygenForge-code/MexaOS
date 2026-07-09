/* ============================================================
 *  MexaOS Timer Driver
 *  PIT-based 1ms tick timer with MexaOS uptime tracking
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>

#include "include/timer.h"
#include "include/interrupt.h"
#include "include/vga.h"
#include "include/serial.h"
#include "include/memory.h"
#include "../../include/mexaos.h"

static volatile uint64_t timer_ticks = 0;
static volatile uint64_t timer_seconds = 0;
static volatile uint64_t timer_ms = 0;

#define MAX_TIMER_CALLBACKS 16
static struct {
    void (*fn)(void);
    uint64_t interval_ms;
    uint64_t last_tick;
    uint8_t active;
} timer_callbacks[MAX_TIMER_CALLBACKS];

static void timer_irq_handler(struct interrupt_frame *frame) {
    (void)frame;
    timer_ticks++;
    timer_ms++;
    if (timer_ms % 1000 == 0) timer_seconds++;
    for (int i = 0; i < MAX_TIMER_CALLBACKS; i++) {
        if (timer_callbacks[i].active) {
            if (timer_ms - timer_callbacks[i].last_tick >= timer_callbacks[i].interval_ms) {
                timer_callbacks[i].last_tick = timer_ms;
                if (timer_callbacks[i].fn) timer_callbacks[i].fn();
            }
        }
    }
    scheduler_tick();
}

void timer_init(uint32_t freq) {
    uint32_t divisor = PIT_FREQUENCY / freq;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    register_irq_handler(0, timer_irq_handler);
    pic_unmask_irq(0);
    timer_ticks = 0; timer_seconds = 0; timer_ms = 0;
    for (int i = 0; i < MAX_TIMER_CALLBACKS; i++) timer_callbacks[i].active = 0;
}

uint64_t timer_get_ticks(void) { return timer_ticks; }
uint64_t timer_get_ms(void) { return timer_ms; }
uint64_t timer_get_seconds(void) { return timer_seconds; }
uint64_t timer_get_uptime(void) { return timer_ticks; }

void timer_sleep_ms(uint64_t ms) {
    uint64_t start = timer_ms;
    while (timer_ms - start < ms) {
        __asm__ __volatile__("pause");
    }
}

void timer_sleep_ticks(uint64_t ticks) {
    uint64_t start = timer_ticks;
    while (timer_ticks - start < ticks) {
        __asm__ __volatile__("pause");
    }
}

int timer_register_callback(void (*fn)(void), uint64_t interval_ms) {
    for (int i = 0; i < MAX_TIMER_CALLBACKS; i++) {
        if (!timer_callbacks[i].active) {
            timer_callbacks[i].fn = fn;
            timer_callbacks[i].interval_ms = interval_ms;
            timer_callbacks[i].last_tick = timer_ms;
            timer_callbacks[i].active = 1;
            return i;
        }
    }
    return -1;
}

void timer_unregister_callback(int id) {
    if (id >= 0 && id < MAX_TIMER_CALLBACKS) timer_callbacks[id].active = 0;
}

void timer_format_uptime(char *buf, size_t size) {
    uint64_t s = timer_seconds;
    uint64_t d = s / 86400; s %= 86400;
    uint64_t h = s / 3600; s %= 3600;
    uint64_t m = s / 60; s %= 60;
    if (d > 0) snprintf(buf, size, "%lud %02lu:%02lu:%02lu", d, h, m, s);
    else snprintf(buf, size, "%02lu:%02lu:%02lu", h, m, s);
}

void timer_timestamp(char *buf, size_t size) {
    uint64_t s = timer_seconds;
    uint64_t h = s / 3600; s %= 3600;
    uint64_t m = s / 60; s %= 60;
    uint64_t ms = timer_ms % 1000;
    snprintf(buf, size, "[%02lu:%02lu:%02lu.%03lu] ", h, m, s, ms);
}
