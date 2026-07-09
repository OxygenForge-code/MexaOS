/* ============================================================
 *  MexaOS Global Header
 * ============================================================ */

#ifndef MEXAOS_H
#define MEXAOS_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* ─── Compiler Attributes ─── */
#define __packed        __attribute__((packed))
#define __aligned(x)    __attribute__((aligned(x)))
#define __noreturn      __attribute__((noreturn))
#define __unused        __attribute__((unused))

/* ─── Inline Assembly Helpers ─── */
#define cli()           __asm__ __volatile__("cli")
#define sti()           __asm__ __volatile__("sti")
#define hlt()           __asm__ __volatile__("hlt")
#define nop()           __asm__ __volatile__("nop")

/* ─── I/O Port Operations ─── */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ __volatile__("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ __volatile__("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t data) {
    __asm__ __volatile__("outw %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ __volatile__("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outl(uint16_t port, uint32_t data) {
    __asm__ __volatile__("outl %0, %1" : : "a"(data), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

/* ─── Memory Barriers ─── */
#define memory_barrier() __asm__ __volatile__("" ::: "memory")

/* ─── Bit Operations ─── */
#define bit_set(x, n)    ((x) |= (1ULL << (n)))
#define bit_clear(x, n)  ((x) &= ~(1ULL << (n)))
#define bit_toggle(x, n) ((x) ^= (1ULL << (n)))
#define bit_test(x, n)   (((x) >> (n)) & 1)

/* ─── Math ─── */
#define min(a, b)        ((a) < (b) ? (a) : (b))
#define max(a, b)        ((a) > (b) ? (a) : (b))
#define abs(x)           ((x) < 0 ? -(x) : (x))

/* ─── Stringify ─── */
#define STR(x)  #x
#define XSTR(x) STR(x)

#endif /* MEXAOS_H */