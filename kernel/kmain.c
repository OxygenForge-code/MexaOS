/* ============================================================
 *  MexaOS Kernel Main — Entry Point
 *  The intent-driven operating system kernel
 * ============================================================ */

#include "../include/mexaos.h"
#include "include/kernlib.h"
#include "include/vga.h"
#include "include/interrupt.h"
#include "include/memory.h"
#include "include/process.h"
#include "include/timer.h"
#include "include/keyboard.h"
#include "include/serial.h"
#include "include/mexfs.h"
#include "include/ai_engine.h"
#include "include/shell.h"
#include "include/gui.h"
#include "include/window.h"

/* ─── Multiboot2 Header ─── */
__attribute__((section(".multiboot")))
static volatile struct {
    uint32_t magic;
    uint32_t arch;
    uint32_t header_len;
    uint32_t checksum;
    uint16_t type;
    uint16_t flags;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint16_t end_type;
    uint16_t end_flags;
    uint32_t end_size;
} multiboot_header = {
    .magic = 0xE85250D6,
    .arch = 0,
    .header_len = 24,
    .checksum = -(0xE85250D6 + 0 + 24),
    .type = 5,
    .flags = 0,
    .size = 20,
    .width = 0,
    .height = 0,
    .depth = 0
};

/* ─── Boot Info from Bootloader ─── */
static struct boot_info {
    uint64_t mem_total;
    uint64_t mem_usable;
    uint32_t fb_addr;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint8_t  fb_bpp;
    char     cmdline[256];
} boot_info;

/* ─── Kernel Entry Point ─── */
void kmain(uint64_t magic, uint64_t mb_info_addr) {
    (void)magic;
    
    /* ─── Phase 0: Minimal VGA output (no heap, no interrupts) ─── */
    vga_early_init();
    vga_clear();
    
    /* MexaOS boot banner */
    vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts("    __  ___                      ____   _____\n");
    vga_puts("   /  |/  /____ _   _____  _____/ __ \\/ ___/\n");
    vga_puts("  / /|_/ // __ \\ | / / _ \\/ ___/ / / /\\__ \\ \n");
    vga_puts(" / /  / // /_/ / |/ /  __/ /  / /_/ /___/ /\n");
    vga_puts("/_/  /_/ \\____/|___/\\___/_/   \\____//____/ \n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("\n    Intent-Driven Operating System v" MEXAOS_VERSION "\n");
    vga_puts("    Build " STR(MEXAOS_BUILD) " — Codename: \"" MEXAOS_CODENAME "\"\n\n");
    
    /* ─── Phase 1: Parse Multiboot Info ─── */
    vga_puts("[BOOT] Parsing boot information... ");
    parse_boot_info(mb_info_addr);
    vga_puts("OK\n");
    
    /* ─── Phase 2: Initialize Serial Port (for debugging) ─── */
    vga_puts("[BOOT] Initializing serial port COM1... ");
    serial_init(COM1_PORT);
    serial_puts(COM1_PORT, "\n=== MexaOS Kernel Debug Log ===\n");
    serial_puts(COM1_PORT, "Version: " MEXAOS_VERSION "\n");
    vga_puts("OK\n");
    
    /* ─── Phase 3: Initialize Memory Management ─── */
    vga_puts("[BOOT] Initializing memory management...\n");
    mem_init(boot_info.mem_total, boot_info.mem_usable);
    vga_puts("       Physical memory: ");
    vga_print_size(boot_info.mem_total);
    vga_puts(" total, ");
    vga_print_size(boot_info.mem_usable);
    vga_puts(" usable\n");
    
    /* ─── Phase 4: Initialize Interrupts ─── */
    vga_puts("[BOOT] Setting up interrupt descriptor table... ");
    idt_init();
    pic_init();
    vga_puts("OK\n");
    
    /* ─── Phase 5: Initialize Timer ─── */
    vga_puts("[BOOT] Calibrating timer (1000Hz)... ");
    timer_init(TIMER_FREQ);
    vga_puts("OK\n");
    
    /* ─── Phase 6: Initialize Keyboard ─── */
    vga_puts("[BOOT] Initializing keyboard driver... ");
    keyboard_init();
    vga_puts("OK\n");
    
    /* Enable interrupts now that core systems are ready */
    sti();
    
    /* ─── Phase 7: Initialize File System (.mex) ─── */
    vga_puts("[BOOT] Initializing Mexa File System (.mex)... ");
    mexfs_init();
    vga_puts("OK\n");
    
    /* ─── Phase 8: Initialize AI Intent Engine ─── */
    vga_puts("[BOOT] Loading AI Intent Engine... ");
    ai_engine_init();
    vga_puts("OK\n");
    
    /* ─── Phase 9: Initialize Process Scheduler ─── */
    vga_puts("[BOOT] Starting process scheduler... ");
    scheduler_init();
    vga_puts("OK\n");
    
    /* ─── Phase 10: Initialize GUI (if framebuffer available) ─── */
    if (boot_info.fb_addr != 0 && boot_info.fb_bpp >= 24) {
        vga_puts("[BOOT] Initializing graphics subsystem... ");
        gui_init(boot_info.fb_addr, boot_info.fb_width, boot_info.fb_height, 
                 boot_info.fb_pitch, boot_info.fb_bpp);
        window_manager_init();
        vga_puts("OK\n");
        
        /* Start GUI shell */
        vga_puts("[BOOT] Starting MexaOS Desktop...\n");
        gui_desktop_start();
    } else {
        /* Text mode: start terminal shell */
        vga_puts("[BOOT] Starting MexaShell (text mode)...\n");
        shell_init();
    }
    
    /* ─── Boot Complete ─── */
    vga_puts("\n");
    vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
    vga_puts("✓ MexaOS boot sequence complete. System ready.\n");
    vga_setcolor(VGA_COLOR_DIM, VGA_COLOR_BLACK);
    vga_puts("  Intent: \"Organize my desktop for coding\" → Try it!\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    serial_puts(COM1_PORT, "[OK] MexaOS boot complete.\n");
    
    /* Enter idle loop — scheduler handles the rest */
    kernel_idle();
}

/* ─── Parse Multiboot2 Information ─── */
static void parse_boot_info(uint64_t addr) {
    struct multiboot_tag *tag = (struct multiboot_tag *)(addr + 8);
    
    boot_info.mem_total = 0;
    boot_info.mem_usable = 0;
    boot_info.fb_addr = 0;
    
    while (tag->type != 0) {
        switch (tag->type) {
            case 4: /* Memory map */
                parse_memory_map(tag);
                break;
            case 5: /* Framebuffer */
                boot_info.fb_addr = ((struct multiboot_tag_framebuffer *)tag)->addr;
                boot_info.fb_width = ((struct multiboot_tag_framebuffer *)tag)->width;
                boot_info.fb_height = ((struct multiboot_tag_framebuffer *)tag)->height;
                boot_info.fb_pitch = ((struct multiboot_tag_framebuffer *)tag)->pitch;
                boot_info.fb_bpp = ((struct multiboot_tag_framebuffer *)tag)->bpp;
                break;
            case 1: /* Command line */
                strncpy(boot_info.cmdline, 
                       ((struct multiboot_tag_string *)tag)->string, 255);
                break;
        }
        tag = (struct multiboot_tag *)ALIGN_UP(
            (uint64_t)tag + tag->size, 8);
    }
}

/* ─── Kernel Idle Loop ─── */
void kernel_idle(void) {
    while (1) {
        /* Halt CPU until next interrupt (power efficient) */
        __asm__ __volatile__(
            "sti\n"
            "hlt\n"
            "cli\n"
        );
        
        /* Process any pending work */
        scheduler_tick();
        ai_process_pending();
    }
}

/* ─── Panic Handler ─── */
void __attribute__((noreturn)) kpanic(const char *msg, const char *file, int line) {
    cli();
    
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_LRED);
    vga_clear();
    vga_puts("\n╔══════════════════════════════════════════════════════════════╗\n");
    vga_puts("║                MEXAOS KERNEL PANIC                           ║\n");
    vga_puts("╠══════════════════════════════════════════════════════════════╣\n");
    
    vga_puts("║  ");
    vga_puts(msg);
    vga_puts("\n║\n");
    
    vga_puts("║  Location: ");
    vga_puts(file);
    vga_puts(":");
    vga_print_dec(line);
    vga_puts("\n║\n");
    
    vga_puts("║  Version: " MEXAOS_VERSION " (Build " STR(MEXAOS_BUILD) ")\n");
    vga_puts("║  Time: ");
    vga_print_dec(timer_get_uptime());
    vga_puts(" ticks since boot\n");
    
    vga_puts("╚══════════════════════════════════════════════════════════════╝\n");
    
    /* Dump register state */
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, rip;
    __asm__ __volatile__("mov %%rax, %0" : "=r"(rax));
    __asm__ __volatile__("mov %%rbx, %0" : "=r"(rbx));
    __asm__ __volatile__("mov %%rcx, %0" : "=r"(rcx));
    __asm__ __volatile__("mov %%rdx, %0" : "=r"(rdx));
    
    vga_puts("\n Registers: RAX=");
    vga_print_hex(rax);
    vga_puts(" RBX=");
    vga_print_hex(rbx);
    vga_puts(" RCX=");
    vga_print_hex(rcx);
    vga_puts(" RDX=");
    vga_print_hex(rdx);
    vga_puts("\n");
    
    /* Halt forever */
    while (1) {
        hlt();
    }
}

/* ─── Kernel Printf ─── */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    
    vga_puts(buf);
    serial_puts(COM1_PORT, buf);
    
    va_end(args);
}
