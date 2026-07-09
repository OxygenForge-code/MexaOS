/* ============================================================
 *  MexaOS Memory Management
 *  Physical memory bitmap allocator + x86_64 paging + slab heap
 * ============================================================ */

#include "include/memory.h"
#include "include/vga.h"
#include "include/interrupt.h"

/* ─── Physical Memory Manager ─── */
static uint8_t *pmm_bitmap = NULL;
static uint64_t pmm_total_blocks = 0;
static uint64_t pmm_used_blocks = 0;
static uint64_t pmm_total_mem = 0;

/* ─── Virtual Memory ─── */
struct page_table kernel_page_table;
static pml4e_t kernel_pml4[512] __attribute__((aligned(PAGE_SIZE)));

/* ─── Heap ─── */
static uint8_t *heap_base = NULL;
static uint8_t *heap_current = NULL;
static uint64_t heap_size = 0;
static uint64_t heap_used = 0;

/* Slab caches */
static struct slab_cache slab_caches[SLAB_NUM_BUCKETS];
static const uint32_t slab_sizes[SLAB_NUM_BUCKETS] = {16, 32, 64, 128, 256, 512, 1024, 2048};

/* ─── Memory Init ─── */
void mem_init(uint64_t mem_total, uint64_t mem_usable) {
    vga_puts("       [PMM] Initializing physical memory bitmap... ");
    pmm_init(mem_total, mem_usable);
    vga_puts("OK\n");
    
    vga_puts("       [VMM] Setting up kernel page tables... ");
    vmm_init();
    vga_puts("OK\n");
    
    vga_puts("       [HEAP] Initializing kernel heap... ");
    heap_init();
    vga_puts("OK\n");
}

void pmm_init(uint64_t mem_total, uint64_t mem_usable) {
    pmm_total_mem = mem_total;
    pmm_total_blocks = mem_total / PMM_BLOCK_SIZE;
    pmm_bitmap = (uint8_t *)phys_to_virt(ALIGN_UP(KERNEL_PHYS_BASE + 0x200000, PAGE_SIZE));
    size_t bitmap_size = DIV_ROUND_UP(pmm_total_blocks, 8);
    memset(pmm_bitmap, 0, bitmap_size);
    pmm_mark_used(0, 2 * 1024 * 1024 / PAGE_SIZE);
    phys_addr_t bitmap_phys = virt_to_phys((virt_addr_t)pmm_bitmap);
    pmm_mark_used(bitmap_phys, DIV_ROUND_UP(bitmap_size, PAGE_SIZE));
    pmm_used_blocks = 2 * 1024 * 1024 / PAGE_SIZE + DIV_ROUND_UP(bitmap_size, PAGE_SIZE);
}

static inline int bitmap_test(uint64_t block) {
    return (pmm_bitmap[block / 8] >> (block % 8)) & 1;
}

static inline void bitmap_set(uint64_t block) {
    pmm_bitmap[block / 8] |= (1 << (block % 8));
}

static inline void bitmap_clear(uint64_t block) {
    pmm_bitmap[block / 8] &= ~(1 << (block % 8));
}

phys_addr_t pmm_alloc_block(void) {
    for (uint64_t i = 0; i < pmm_total_blocks; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_used_blocks++;
            return i * PMM_BLOCK_SIZE;
        }
    }
    return 0;
}

phys_addr_t pmm_alloc_blocks(size_t count) {
    uint64_t start = 0, found = 0;
    for (uint64_t i = 0; i < pmm_total_blocks; i++) {
        if (!bitmap_test(i)) {
            if (found == 0) start = i;
            found++;
            if (found >= count) {
                for (uint64_t j = start; j < start + count; j++) bitmap_set(j);
                pmm_used_blocks += count;
                return start * PMM_BLOCK_SIZE;
            }
        } else {
            found = 0;
        }
    }
    return 0;
}

void pmm_free_block(phys_addr_t addr) {
    uint64_t block = addr / PMM_BLOCK_SIZE;
    if (block < pmm_total_blocks && bitmap_test(block)) {
        bitmap_clear(block);
        pmm_used_blocks--;
    }
}

void pmm_free_blocks(phys_addr_t addr, size_t count) {
    uint64_t start = addr / PMM_BLOCK_SIZE;
    for (uint64_t i = start; i < start + count && i < pmm_total_blocks; i++) {
        if (bitmap_test(i)) {
            bitmap_clear(i);
            pmm_used_blocks--;
        }
    }
}

void pmm_mark_used(phys_addr_t addr, size_t pages) {
    uint64_t start = addr / PMM_BLOCK_SIZE;
    for (uint64_t i = start; i < start + pages; i++) {
        if (i < pmm_total_blocks && !bitmap_test(i)) {
            bitmap_set(i);
            pmm_used_blocks++;
        }
    }
}

void pmm_mark_free(phys_addr_t addr, size_t pages) {
    uint64_t start = addr / PMM_BLOCK_SIZE;
    for (uint64_t i = start; i < start + pages; i++) {
        if (i < pmm_total_blocks && bitmap_test(i)) {
            bitmap_clear(i);
            pmm_used_blocks--;
        }
    }
}

uint64_t pmm_get_free(void) {
    return (pmm_total_blocks - pmm_used_blocks) * PMM_BLOCK_SIZE;
}

uint64_t pmm_get_used(void) {
    return pmm_used_blocks * PMM_BLOCK_SIZE;
}

/* ─── Virtual Memory ─── */
void vmm_init(void) {
    kernel_page_table.pml4 = kernel_pml4;
    memset(kernel_pml4, 0, PAGE_SIZE);
    
    pdpte_t *pdpt = (pdpte_t *)pmm_alloc_block();
    memset(pdpt, 0, PAGE_SIZE);
    kernel_pml4[0] = (pml4e_t)pdpt | PTE_PRESENT | PTE_WRITABLE;
    
    pdpte_t *kernel_pdpt = (pdpte_t *)pmm_alloc_block();
    memset(kernel_pdpt, 0, PAGE_SIZE);
    uint64_t pml4_idx = (KERNEL_VIRT_BASE >> 39) & 0x1FF;
    kernel_pml4[pml4_idx] = (pml4e_t)kernel_pdpt | PTE_PRESENT | PTE_WRITABLE;
    
    pde_t *kernel_pd = (pde_t *)pmm_alloc_block();
    memset(kernel_pd, 0, PAGE_SIZE);
    kernel_pdpt[0] = (pdpte_t)kernel_pd | PTE_PRESENT | PTE_WRITABLE;
    
    for (int i = 0; i < 64; i++) {
        phys_addr_t phys = i * HUGE_PAGE_SIZE;
        kernel_pd[i] = phys | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE;
    }
    
    uint64_t cr0, cr4;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x10000;
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
    
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x20;
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));
    
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(kernel_pml4));
}

void vmm_switch_page_table(struct page_table *pt) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(pt->pml4));
}

struct page_table *vmm_create_page_table(void) {
    struct page_table *pt = kmalloc(sizeof(struct page_table));
    if (!pt) return NULL;
    pt->pml4 = (pml4e_t *)pmm_alloc_block();
    if (!pt->pml4) { kfree(pt); return NULL; }
    memset(pt->pml4, 0, PAGE_SIZE);
    uint64_t pml4_idx = (KERNEL_VIRT_BASE >> 39) & 0x1FF;
    pt->pml4[pml4_idx] = kernel_pml4[pml4_idx];
    return pt;
}

void vmm_destroy_page_table(struct page_table *pt) {
    if (!pt) return;
    pmm_free_block((phys_addr_t)pt->pml4);
    kfree(pt);
}

int vmm_map_page(struct page_table *pt, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FF;
    
    pml4e_t *pml4 = pt->pml4;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        phys_addr_t pdpt_phys = pmm_alloc_block();
        if (!pdpt_phys) return -1;
        memset(phys_to_virt(pdpt_phys), 0, PAGE_SIZE);
        pml4[pml4_idx] = pdpt_phys | PTE_PRESENT | PTE_WRITABLE;
    }
    pdpte_t *pdpt = (pdpte_t *)phys_to_virt(pml4[pml4_idx] & PTE_ADDR_MASK);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        phys_addr_t pd_phys = pmm_alloc_block();
        if (!pd_phys) return -1;
        memset(phys_to_virt(pd_phys), 0, PAGE_SIZE);
        pdpt[pdpt_idx] = pd_phys | PTE_PRESENT | PTE_WRITABLE;
    }
    pde_t *pd = (pde_t *)phys_to_virt(pdpt[pdpt_idx] & PTE_ADDR_MASK);
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        phys_addr_t pt_phys = pmm_alloc_block();
        if (!pt_phys) return -1;
        memset(phys_to_virt(pt_phys), 0, PAGE_SIZE);
        pd[pd_idx] = pt_phys | PTE_PRESENT | PTE_WRITABLE;
    }
    pte_t *ptable = (pte_t *)phys_to_virt(pd[pd_idx] & PTE_ADDR_MASK);
    ptable[pt_idx] = (paddr & PTE_ADDR_MASK) | flags | PTE_PRESENT;
    invlpg(vaddr);
    return 0;
}

/* ─── Heap ─── */
void heap_init(void) {
    heap_size = KHEAP_SIZE;
    heap_base = (uint8_t *)KHEAP_START;
    heap_current = heap_base;
    heap_used = 0;
    for (int i = 0; i < SLAB_NUM_BUCKETS; i++) {
        slab_caches[i].object_size = slab_sizes[i];
        slab_caches[i].objects_per_slab = PAGE_SIZE / slab_sizes[i];
        slab_caches[i].partial = NULL;
        slab_caches[i].full = NULL;
        slab_caches[i].free = NULL;
    }
}

static void *heap_bump_alloc(size_t size) {
    size_t aligned_size = ALIGN_UP(size, 16);
    if ((uint64_t)(heap_current - heap_base) + aligned_size > heap_size) return NULL;
    void *ptr = heap_current;
    heap_current += aligned_size;
    heap_used += aligned_size;
    return ptr;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    for (int i = 0; i < SLAB_NUM_BUCKETS; i++) {
        if (size <= slab_sizes[i]) return heap_bump_alloc(slab_sizes[i]);
    }
    return heap_bump_alloc(ALIGN_UP(size, PAGE_SIZE));
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr) { (void)ptr; }

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }
    void *new_ptr = kmalloc(new_size);
    if (new_ptr) { memcpy(new_ptr, ptr, new_size); kfree(ptr); }
    return new_ptr;
}

void *kmemdup(const void *src, size_t size) {
    void *dst = kmalloc(size);
    if (dst) memcpy(dst, src, size);
    return dst;
}

void *kvalloc(size_t size) { return kmalloc(ALIGN_UP(size, PAGE_SIZE)); }
void kvfree(void *ptr, size_t size) { (void)ptr; (void)size; }

/* ─── Memory Info ─── */
void mem_get_info(struct mem_info *info) {
    if (!info) return;
    info->total = pmm_total_mem;
    info->usable = pmm_total_mem - 0x200000;
    info->used = pmm_get_used();
    info->free = pmm_get_free();
    info->kernel = 0x200000;
    info->heap_used = heap_used;
    info->slab_used = 0;
}

/* ─── String Functions ─── */
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest; const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = dest; const uint8_t *s = src;
    if (d < s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1, *p2 = s2;
    while (n--) { if (*p1 != *p2) return *p1 - *p2; p1++; p2++; }
    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? (*(unsigned char *)s1 - *(unsigned char *)s2) : 0;
}

size_t strlen(const char *s) { size_t len = 0; while (s[len]) len++; return len; }

char *strcpy(char *dest, const char *src) {
    char *d = dest; while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    if (n) while (--n) *d++ = '\0';
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest; while (*d) d++; while ((*d++ = *src++));
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) { if (*s == c) return (char *)s; s++; }
    return NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) { if (*s == c) last = s; s++; }
    return (char *)last;
}

int atoi(const char *s) {
    int sign = 1, val = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; }
    return sign * val;
}

char *itoa(int value, char *str, int base) {
    char *rc = str, *ptr = str, *low;
    if (base < 2 || base > 36) { *str = '\0'; return str; }
    if (value < 0 && base == 10) *ptr++ = '-';
    low = ptr;
    do {
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
        value /= base;
    } while (value);
    *ptr-- = '\0';
    while (low < ptr) { char tmp = *low; *low++ = *ptr; *ptr-- = tmp; }
    return rc;
}

char *uitoa(uint64_t value, char *str, int base) {
    char *rc = str, *ptr = str, *low = str;
    if (base < 2 || base > 36) { *str = '\0'; return str; }
    do {
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
        value /= base;
    } while (value);
    *ptr-- = '\0';
    while (low < ptr) { char tmp = *low; *low++ = *ptr; *ptr-- = tmp; }
    return rc;
}

/* ─── printf ─── */
static int print_int(char *buf, size_t size, size_t *pos, int64_t val, int base, int pad, char pad_char, int uppercase) {
    char tmp[64];
    int len = 0, negative = 0;
    if (val < 0 && base == 10) { negative = 1; val = -val; }
    do {
        int digit = val % base;
        tmp[len++] = uppercase ? "0123456789ABCDEF"[digit] : "0123456789abcdef"[digit];
        val /= base;
    } while (val > 0);
    int total_len = len + negative;
    int padding = pad > total_len ? pad - total_len : 0;
    if (negative && *pos < size - 1) buf[(*pos)++] = '-';
    for (int i = 0; i < padding && *pos < size - 1; i++) buf[(*pos)++] = pad_char;
    for (int i = len - 1; i >= 0 && *pos < size - 1; i--) buf[(*pos)++] = tmp[i];
    return *pos;
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list args) {
    size_t pos = 0;
    while (*fmt && pos < size - 1) {
        if (*fmt != '%') { str[pos++] = *fmt++; continue; }
        fmt++;
        int pad = 0; char pad_char = ' '; int long_long = 0; int uppercase = 0;
        if (*fmt == '0') { pad_char = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { pad = pad * 10 + (*fmt - '0'); fmt++; }
        if (*fmt == 'l') { fmt++; if (*fmt == 'l') { long_long = 1; fmt++; } }
        switch (*fmt) {
            case 'd': case 'i': {
                int64_t val = long_long ? va_arg(args, long long) : va_arg(args, int);
                print_int(str, size, &pos, val, 10, pad, pad_char, 0);
                break;
            }
            case 'u': {
                uint64_t val = long_long ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
                print_int(str, size, &pos, val, 10, pad, pad_char, 0);
                break;
            }
            case 'x': case 'X': {
                uppercase = (*fmt == 'X');
                uint64_t val = long_long ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
                if (pos < size - 1) str[pos++] = '0';
                if (pos < size - 1) str[pos++] = uppercase ? 'X' : 'x';
                print_int(str, size, &pos, val, 16, pad, pad_char, uppercase);
                break;
            }
            case 'p': {
                if (pos < size - 1) str[pos++] = '0';
                if (pos < size - 1) str[pos++] = 'x';
                uint64_t val = (uint64_t)va_arg(args, void *);
                print_int(str, size, &pos, val, 16, 16, '0', 0);
                break;
            }
            case 'c': { char c = (char)va_arg(args, int); str[pos++] = c; break; }
            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                while (*s && pos < size - 1) str[pos++] = *s++;
                break;
            }
            case '%': str[pos++] = '%'; break;
            default: str[pos++] = *fmt; break;
        }
        fmt++;
    }
    if (pos < size) str[pos] = '\0';
    else if (size > 0) str[size - 1] = '\0';
    return (int)pos;
}

int snprintf(char *str, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(str, size, fmt, args);
    va_end(args);
    return ret;
}
