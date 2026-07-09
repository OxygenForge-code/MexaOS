/* ============================================================
 *  MexaOS Memory Manager
 * ============================================================ */

#ifndef MEXAOS_MEMORY_H
#define MEXAOS_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>          /* EKLENDİ: va_list için */

/* ─── Physical Memory ─── */
#define PAGE_SIZE       4096
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x)   (((x) + PAGE_SIZE - 1) & PAGE_MASK)

/* ─── Virtual Memory ─── */
#define KERNEL_BASE     0xFFFFFFFF80000000ULL
#define PHYS_TO_VIRT(p) ((void *)((uint64_t)(p) + KERNEL_BASE))
#define VIRT_TO_PHYS(v) ((uint64_t)(v) - KERNEL_BASE)

/* ─── Memory Map Entry ─── */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} memory_map_entry_t;

/* ─── Functions ─── */
void mm_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t size);

/* String functions */
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);

/* Printf functions */
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);

#endif /* MEXAOS_MEMORY_H */
