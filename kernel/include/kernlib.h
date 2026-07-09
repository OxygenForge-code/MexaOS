/* ============================================================
 *  MexaOS Kernel Library Header
 *  Common utilities used across kernel modules
 * ============================================================ */

#ifndef KERNLIB_H
#define KERNLIB_H

#include "../../include/mexaos.h"

/* Multiboot2 tag types */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[];
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
};

/* Boot info parsing */
void parse_boot_info(uint64_t addr);
void parse_memory_map(struct multiboot_tag *tag);

/* Kernel functions */
void kernel_idle(void);
void kprintf(const char *fmt, ...);
void __attribute__((noreturn)) kpanic(const char *msg, const char *file, int line);

#define PANIC(msg) kpanic(msg, __FILE__, __LINE__)

#endif /* KERNLIB_H */
