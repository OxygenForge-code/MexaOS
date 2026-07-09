/* ============================================================
 *  MexaOS Memory Manager
 * ============================================================ */

#ifndef MEXAOS_MEMORY_H
#define MEXAOS_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* ─── Physical Memory ─── */
#define PAGE_SIZE       4096
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x)   (((x) + PAGE_SIZE - 1) & PAGE_MASK)

/* ─── Virtual Memory ─── */
#define KERNEL_BASE     0xFFFFFFFF80000000ULL
#define KERNEL_PHYS_BASE 0x200000
#define KERNEL_VIRT_BASE KERNEL_BASE
#define PHYS_TO_VIRT(p) ((void *)((uint64_t)(p) + KERNEL_BASE))
#define VIRT_TO_PHYS(v) ((uint64_t)(v) - KERNEL_BASE)

/* ─── Page Table Types ─── */
typedef uint64_t pml4e_t;
typedef uint64_t pdpte_t;
typedef uint64_t pde_t;
typedef uint64_t pte_t;
typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

/* ─── Page Flags ─── */
#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_HUGE        (1ULL << 7)
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

/* ─── Huge Page ─── */
#define HUGE_PAGE_SIZE  (2 * 1024 * 1024)

/* ─── PMM ─── */
#define PMM_BLOCK_SIZE  PAGE_SIZE

/* ─── Heap ─── */
#define KHEAP_START     (KERNEL_BASE + 0x1000000)
#define KHEAP_SIZE      (16 * 1024 * 1024)

/* ─── Slab ─── */
#define SLAB_NUM_BUCKETS 8

/* ─── Memory Map Entry ─── */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} memory_map_entry_t;

/* ─── Memory Info ─── */
struct mem_info {
    uint64_t total;
    uint64_t usable;
    uint64_t used;
    uint64_t free;
    uint64_t kernel;
    uint64_t heap_used;
    uint64_t slab_used;
};

/* ─── Page Table ─── */
struct page_table {
    pml4e_t *pml4;
};

extern struct page_table kernel_page_table;

/* ─── Slab Cache ─── */
struct slab_cache {
    uint32_t size;
    uint32_t object_size;
    uint32_t objects_per_slab;
    uint32_t count;
    void *head;
    void *partial;
    void *full;
    void *free;
};

/* ─── Functions ─── */
void mm_init(void);
void mem_init(uint64_t mem_total, uint64_t mem_usable);

void pmm_init(uint64_t mem_total, uint64_t mem_usable);
phys_addr_t pmm_alloc_block(void);
phys_addr_t pmm_alloc_blocks(size_t count);
void pmm_free_block(phys_addr_t addr);
void pmm_free_blocks(phys_addr_t addr, size_t count);
void pmm_mark_used(phys_addr_t addr, size_t pages);
void pmm_mark_free(phys_addr_t addr, size_t pages);
uint64_t pmm_get_free(void);
uint64_t pmm_get_used(void);

void vmm_init(void);
void vmm_switch_page_table(struct page_table *pt);
struct page_table *vmm_create_page_table(void);
void vmm_destroy_page_table(struct page_table *pt);
int vmm_map_page(struct page_table *pt, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
virt_addr_t vmm_alloc_region(struct page_table *pt, size_t pages, uint64_t flags);

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t size);

void mem_get_info(struct mem_info *info);

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

/* Macros */
#define ALIGN_UP(x, a)    (((x) + (a) - 1) & ~((a) - 1))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

static inline void *phys_to_virt(phys_addr_t p) {
    return (void *)(p + KERNEL_BASE);
}

static inline phys_addr_t virt_to_phys(void *v) {
    return (phys_addr_t)v - KERNEL_BASE;
}

static inline void invlpg(void *addr) {
    __asm__ __volatile__("invlpg (%0)" ::"r"(addr) : "memory");
}

#endif
