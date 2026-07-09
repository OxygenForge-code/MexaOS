/* ============================================================
 *  MexaOS Kernel Main
 *  Entry point from bootloader (64-bit Long Mode)
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "include/kernlib.h"
#include "include/vga.h"
#include "include/interrupt.h"
#include "include/memory.h"
#include "include/timer.h"
#include "include/keyboard.h"
#include "include/serial.h"
#include "include/process.h"
#include "include/mexfs.h"
#include "include/ai_engine.h"
#include "include/shell.h"
#include "include/window.h"

/* Build info defaults */
#ifndef MEXAOS_VERSION
#define MEXAOS_VERSION "2.0.0"
#endif
#ifndef MEXAOS_BUILD
#define MEXAOS_BUILD 0
#endif
#ifndef MEXAOS_CODENAME
#define MEXAOS_CODENAME "Unknown"
#endif

/* Stringify macros */
#define STR(x)  #x
#define XSTR(x) STR(x)

/* ─── Kernel Entry Point ─── */
void kmain(uint64_t boot_info_addr) {
    (void)boot_info_addr;
    
    vga_init();
    vga_clear();
    
    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("MexaOS v");
    vga_puts(MEXAOS_VERSION);
    vga_puts(" Build ");
    vga_puts(XSTR(MEXAOS_BUILD));
    vga_puts("\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_puts("[*] Initializing memory manager...\n");
    mm_init();
    
    vga_puts("[*] Initializing interrupt descriptor table...\n");
    idt_init();
    
    vga_puts("[*] Initializing timer (PIT 1000Hz)...\n");
    timer_init(1000);
    
    vga_puts("[*] Initializing keyboard driver...\n");
    keyboard_init();
    
    vga_puts("[*] Initializing serial port (COM1)...\n");
    serial_init(SERIAL_COM1);
    
    vga_puts("[*] Initializing process scheduler...\n");
    scheduler_init();
    
    vga_puts("[*] Initializing MexFS filesystem...\n");
    mexfs_init();
    
    vga_puts("[*] Initializing AI Engine...\n");
    ai_engine_init();
    
    vga_puts("[*] Initializing window manager...\n");
    wm_init();
    
    vga_puts("\n[*] Starting MexaOS Shell...\n\n");
    shell_run();
    
    kpanic("Shell exited unexpectedly");
}

/* ─── Parse Boot Info ─── */
void parse_boot_info(uint64_t addr) {
    (void)addr;
    /* TODO: Parse memory map, framebuffer info, etc. */
}

/* ─── Kernel Panic Implementation ─── */
void __attribute__((noreturn)) kpanic_impl(const char *msg, const char *file, int line) {
    cli();
    
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();
    
    vga_puts("╔══════════════════════════════════════════════════════════╗\n");
    vga_puts("║                    KERNEL PANIC                            ║\n");
    vga_puts("╠══════════════════════════════════════════════════════════╣\n");
    vga_puts("║  Message: ");
    vga_puts(msg);
    vga_puts("\n");
    vga_puts("║  File:    ");
    vga_puts(file);
    vga_puts("\n");
    vga_puts("║  Line:    ");
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", line);
    vga_puts(buf);
    
    vga_puts("\n");
    vga_puts("║  Version: ");
    vga_puts(MEXAOS_VERSION);
    vga_puts(" (Build ");
    vga_puts(XSTR(MEXAOS_BUILD));
    vga_puts(")\n");
    vga_puts("╚══════════════════════════════════════════════════════════╝\n");
    
    while (1) {
        hlt();
    }
}

/* ─── Formatted Print ─── */
void kprintf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    vga_puts(buf);
}
