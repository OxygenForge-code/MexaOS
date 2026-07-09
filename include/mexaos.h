/* ============================================================
 *  MexaOS Kernel Header — Core Definitions
 *  The intent-driven operating system
 * ============================================================ */

#ifndef MEXAOS_H
#define MEXAOS_H

/* ─── Compiler & Architecture ─── */
#define __mexaos_kernel__ 1
#define ARCH_X86_64 1

/* ─── MexaOS Version ─── */
#define MEXAOS_NAME        "MexaOS"
#define MEXAOS_VERSION     "2.0.0"
#define MEXAOS_CODENAME    "Intent"
#define MEXAOS_BUILD       2240
#define MEXAOS_BUILD_DATE  "2026-07-09"

/* ─── Memory Layout ─── */
#define KERNEL_VIRT_BASE    0xFFFFFFFF80000000ULL
#define KERNEL_PHYS_BASE    0x100000
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define HUGE_PAGE_SIZE      (2 * 1024 * 1024)  /* 2MB */

#define STACK_SIZE          (16 * PAGE_SIZE)   /* 64KB kernel stack */
#define STACK_TOP           0x200000

/* Physical memory regions */
#define PHYS_MEM_START      0x100000            /* 1MB */
#define PHYS_MEM_END        0x100000000ULL      /* 4GB max */

/* Heap */
#define KHEAP_START         0xFFFF900000000000ULL
#define KHEAP_SIZE          (16 * 1024 * 1024)  /* 16MB initial */

/* ─── VGA Display ─── */
#define VGA_BUFFER          0xB8000
#define VGA_COLS            80
#define VGA_ROWS            25
#define VGA_COLOR_BLACK     0
#define VGA_COLOR_BLUE      1
#define VGA_COLOR_GREEN     2
#define VGA_COLOR_CYAN      3
#define VGA_COLOR_RED       4
#define VGA_COLOR_MAGENTA   5
#define VGA_COLOR_BROWN     6
#define VGA_COLOR_LGRAY     7
#define VGA_COLOR_DGRAY     8
#define VGA_COLOR_LBLUE     9
#define VGA_COLOR_LGREEN    10
#define VGA_COLOR_LCYAN     11
#define VGA_COLOR_LRED      12
#define VGA_COLOR_LMAGENTA  13
#define VGA_COLOR_YELLOW    14
#define VGA_COLOR_WHITE     15

/* MexaOS Theme Colors (matches website) */
#define MEXA_COLOR_BG       VGA_COLOR_BLACK
#define MEXA_COLOR_FG       VGA_COLOR_WHITE
#define MEXA_COLOR_ACCENT   VGA_COLOR_LBLUE     /* Indigo accent */
#define MEXA_COLOR_DIM      VGA_COLOR_DGRAY
#define MEXA_COLOR_SUCCESS  VGA_COLOR_LGREEN
#define MEXA_COLOR_WARN     VGA_COLOR_YELLOW
#define MEXA_COLOR_ERROR    VGA_COLOR_LRED

/* ─── Serial Port ─── */
#define COM1_PORT           0x3F8

/* ─── Keyboard ─── */
#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_STATUS_PORT 0x64

/* ─── PIC (Programmable Interrupt Controller) ─── */
#define PIC1_COMMAND        0x20
#define PIC1_DATA           0x21
#define PIC2_COMMAND        0xA0
#define PIC2_DATA           0xA1
#define PIC_EOI             0x20
#define ICW1_INIT           0x11
#define ICW4_8086           0x01

/* ─── Interrupts ─── */
#define IDT_ENTRIES         256
#define IRQ0_VECTOR         0x20
#define IRQ8_VECTOR         0x28
#define INT_SYSCALL         0x80

/* ─── Timer ─── */
#define PIT_FREQUENCY       1193182
#define TIMER_FREQ          1000        /* 1ms ticks = 1000Hz */
#define PIT_DIVISOR         (PIT_FREQUENCY / TIMER_FREQ)

/* ─── Process ─── */
#define MAX_PROCESSES       256
#define PROC_NAME_LEN       32
#define KERNEL_PID          0
#define INIT_PID            1

/* Process states */
#define PROC_EMPTY          0
#define PROC_READY          1
#define PROC_RUNNING        2
#define PROC_BLOCKED        3
#define PROC_ZOMBIE         4
#define PROC_SLEEPING       5

/* ─── File System (.mex) ─── */
#define MEX_MAGIC           0x4D455841  /* "MEXA" */
#define MEX_VERSION         1
#define MEX_BLOCK_SIZE      4096
#define MEX_MAX_FILES       65536
#define MEX_MAX_NAME        255
#define MEX_MAX_PATH        4096
#define MEX_SECTOR_SIZE     512

/* File types */
#define MEX_TYPE_REGULAR    0
#define MEX_TYPE_DIRECTORY  1
#define MEX_TYPE_SYMLINK    2
#define MEX_TYPE_DEVICE     3
#define MEX_TYPE_PIPE       4
#define MEX_TYPE_AI_MODEL   5       /* MexaOS special: AI model file */
#define MEX_TYPE_INTENT     6       /* MexaOS special: intent script */

/* Open modes */
#define MEX_O_READ          0x01
#define MEX_O_WRITE         0x02
#define MEX_O_CREATE        0x04
#define MEX_O_APPEND        0x08
#define MEX_O_TRUNC         0x10
#define MEX_O_EXEC          0x20    /* Executable intent */

/* Permissions */
#define MEX_PERM_OWNER_R    0x100
#define MEX_PERM_OWNER_W    0x080
#define MEX_PERM_OWNER_X    0x040
#define MEX_PERM_GROUP_R    0x020
#define MEX_PERM_GROUP_W    0x010
#define MEX_PERM_GROUP_X    0x008
#define MEX_PERM_OTHER_R    0x004
#define MEX_PERM_OTHER_W    0x002
#define MEX_PERM_OTHER_X    0x001

/* ─── AI Intent Engine ─── */
#define INTENT_MAX_LEN      4096
#define INTENT_MAX_TOKENS   256
#define INTENT_MAX_ACTIONS  32
#define INTENT_LEARN_FILE   "/system/ai/intent_memory.mex"

/* Intent categories */
#define INTENT_UNKNOWN      0
#define INTENT_OPEN         1
#define INTENT_CLOSE        2
#define INTENT_CREATE       3
#define INTENT_DELETE       4
#define INTENT_MOVE         5
#define INTENT_COPY         6
#define INTENT_SEARCH       7
#define INTENT_SYSTEM       8
#define INTENT_CONFIGURE    9
#define INTENT_QUERY        10
#define INTENT_EXECUTE      11
#define INTENT_AUTOMATE     12
#define INTENT_LEARN        13
#define INTENT_WORKSPACE    14
#define INTENT_FOCUS        15
#define INTENT_CREATIVE     16
#define INTENT_CALM         17
#define INTENT_CODING       18
#define INTENT_COMMUNICATE  19

/* ─── GUI ─── */
#define GUI_FB_WIDTH        1920
#define GUI_FB_HEIGHT       1080
#define GUI_FB_BPP          32
#define GUI_FB_SIZE         (GUI_FB_WIDTH * GUI_FB_HEIGHT * 4)

/* Window manager */
#define MAX_WINDOWS         64
#define WIN_TITLE_LEN       64
#define WIN_BORDER_WIDTH    1
#define WIN_TITLE_HEIGHT    30

/* ─── Types ─── */
#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef unsigned long       size_t;
typedef long                ssize_t;
#endif

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef uint64_t            phys_addr_t;
typedef uint64_t            virt_addr_t;
typedef uint64_t            pml4e_t;
typedef uint64_t            pdpte_t;
typedef uint64_t            pde_t;
typedef uint64_t            pte_t;

typedef int32_t             pid_t;
typedef int32_t             fid_t;      /* File ID */
typedef uint32_t            mode_t;
typedef uint64_t            time_t;

/* ─── Inline Assembly Helpers ─── */
#define cli()               __asm__ __volatile__("cli")
#define sti()               __asm__ __volatile__("sti")
#define hlt()               __asm__ __volatile__("hlt")
#define pause()             __asm__ __volatile__("pause")
#define invlpg(addr)        __asm__ __volatile__("invlpg (%0)" :: "r"(addr))

#define inb(port)           ({ uint8_t _v; __asm__ __volatile__("inb %1, %0" : "=a"(_v) : "Nd"(port)); _v; })
#define outb(port, val)     __asm__ __volatile__("outb %0, %1" :: "a"((uint8_t)(val)), "Nd"(port))
#define inw(port)           ({ uint16_t _v; __asm__ __volatile__("inw %1, %0" : "=a"(_v) : "Nd"(port)); _v; })
#define outw(port, val)     __asm__ __volatile__("outw %0, %1" :: "a"((uint16_t)(val)), "Nd"(port))
#define inl(port)           ({ uint32_t _v; __asm__ __volatile__("inl %1, %0" : "=a"(_v) : "Nd"(port)); _v; })
#define outl(port, val)     __asm__ __volatile__("outl %0, %1" :: "a"((uint32_t)(val)), "Nd"(port))

#define rdmsr(msr, lo, hi)  __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr))
#define wrmsr(msr, lo, hi)  __asm__ __volatile__("wrmsr" :: "a"(lo), "d"(hi), "c"(msr))

/* ─── Memory Barriers ─── */
#define mb()                __asm__ __volatile__("mfence" ::: "memory")
#define rmb()               __asm__ __volatile__("lfence" ::: "memory")
#define wmb()               __asm__ __volatile__("sfence" ::: "memory")

/* ─── Assert ─── */
#define ASSERT(cond) \
    do { if (!(cond)) { kprintf("ASSERT failed: " #cond " at " __FILE__ ":%d\n", __LINE__); while(1) hlt(); } } while(0)

/* ─── Macros ─── */
#define ALIGN_UP(x, a)      (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))
#define DIV_ROUND_UP(x, a)  (((x) + (a) - 1) / (a))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define ABS(x)              ((x) < 0 ? -(x) : (x))
#define BIT(n)              (1ULL << (n))
#define IS_ALIGNED(x, a)    (((x) & ((a) - 1)) == 0)
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* ─── NULL ─── */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* MEXAOS_H */
