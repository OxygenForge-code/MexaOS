/* ============================================================
 *  MexaOS Memory Management Header
 *  Physical allocator, virtual memory (paging), heap
 * ============================================================ */

#ifndef MEMORY_H
#define MEMORY_H

#include "../../include/mexaos.h"

/* ─── Page Table Entry Flags ─── */
#define PTE_PRESENT     0x001
#define PTE_WRITABLE    0x002
#define PTE_USER        0x004
#define PTE_WRITETHROUGH 0x008
#define PTE_NOCACHE     0x010
#define PTE_ACCESSED    0x020
#define PTE_DIRTY       0x040
#define PTE_HUGE        0x080
#define PTE_GLOBAL      0x100
#define PTE_NX          (1ULL << 63)

#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

/* ─── Physical Memory Manager ─── */
#define PMM_BLOCK_SIZE      PAGE_SIZE
#define PMM_BLOCKS_PER_BYTE 8
#define PMM_BITMAP_MAX      (4ULL * 1024 * 1024 * 1024 / PAGE_SIZE / 8) /* 4GB */

void pmm_init(uint64_t mem_total, uint64_t mem_usable);
phys_addr_t pmm_alloc_block(void);
phys_addr_t pmm_alloc_blocks(size_t count);
void pmm_free_block(phys_addr_t addr);
void pmm_free_blocks(phys_addr_t addr, size_t count);
void pmm_mark_used(phys_addr_t addr, size_t pages);
void pmm_mark_free(phys_addr_t addr, size_t pages);
uint64_t pmm_get_free(void);
uint64_t pmm_get_used(void);
void pmm_dump_state(void);

/* ─── Virtual Memory Manager (Paging) ─── */
struct page_table {
    pml4e_t *pml4;
};

extern struct page_table kernel_page_table;

void vmm_init(void);
void vmm_switch_page_table(struct page_table *pt);
struct page_table *vmm_create_page_table(void);
void vmm_destroy_page_table(struct page_table *pt);
int vmm_map_page(struct page_table *pt, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
int vmm_map_pages(struct page_table *pt, virt_addr_t vaddr, phys_addr_t paddr, size_t count, uint64_t flags);
int vmm_unmap_page(struct page_table *pt, virt_addr_t vaddr);
phys_addr_t vmm_get_phys(struct page_table *pt, virt_addr_t vaddr);
virt_addr_t vmm_alloc_region(struct page_table *pt, size_t pages, uint64_t flags);
void vmm_free_region(struct page_table *pt, virt_addr_t vaddr, size_t pages);
void vmm_map_kernel_higher_half(struct page_table *pt);

/* Kernel virtual memory helpers */
static inline virt_addr_t phys_to_virt(phys_addr_t paddr) {
    return paddr + KERNEL_VIRT_BASE;
}

static inline phys_addr_t virt_to_phys(virt_addr_t vaddr) {
    return vaddr - KERNEL_VIRT_BASE;
}

/* ─── Heap / Slab Allocator ─── */
#define SLAB_MIN_SIZE       16
#define SLAB_MAX_SIZE       2048
#define SLAB_NUM_BUCKETS    8       /* 16, 32, 64, 128, 256, 512, 1024, 2048 */
#define SLAB_OBJECTS_PER_PAGE   (PAGE_SIZE / SLAB_MIN_SIZE)

struct slab_block {
    struct slab_block *next;
    uint32_t magic;
    uint32_t size;
};

struct slab_cache {
    uint32_t object_size;
    uint32_t objects_per_slab;
    struct slab *partial;
    struct slab *full;
    struct slab *free;
};

void heap_init(void);
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
void kfree(void *ptr);
void *kmemdup(const void *src, size_t size);
void *kvalloc(size_t size);         /* Page-aligned allocation */
void kvfree(void *ptr, size_t size);

/* String functions */
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int atoi(const char *s);
char *itoa(int value, char *str, int base);
char *uitoa(uint64_t value, char *str, int base);

/* printf family */
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);

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

void mem_get_info(struct mem_info *info);
void mem_init(uint64_t mem_total, uint64_t mem_usable);

#endif /* MEMORY_H */
