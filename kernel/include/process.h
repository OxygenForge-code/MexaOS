2026-07-09T18:26:40.6553986Z ##[group]Run make clean
make clean
make all
shell: /usr/bin/bash -e {0}
  CLEAN build files
  AS    kernel/isr.asm
  CC    kernel/kmain.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
  CC    kernel/vga.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
  CC    kernel/interrupt.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
  CC    kernel/memory.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
kernel/memory.c: In function ‘pmm_init’:
kernel/memory.c:52:44: warning: passing argument 1 of ‘virt_to_phys’ makes pointer from integer without a cast [-Wint-conversion]
   52 |     phys_addr_t bitmap_phys = virt_to_phys((virt_addr_t)pmm_bitmap);
      |                                            ^~~~~~~~~~~~~~~~~~~~~~~
      |                                            |
      |                                            long unsigned int
In file included from kernel/memory.c:6:
kernel/include/memory.h:140:46: note: expected ‘void *’ but argument is of type ‘long unsigned int’
  140 | static inline phys_addr_t virt_to_phys(void *v) {
      |                                        ~~~~~~^
kernel/memory.c:45:44: warning: unused parameter ‘mem_usable’ [-Wunused-parameter]
   45 | void pmm_init(uint64_t mem_total, uint64_t mem_usable) {
      |                                   ~~~~~~~~~^~~~~~~~~~
kernel/memory.c: In function ‘vmm_map_page’:
kernel/memory.c:229:12: warning: passing argument 1 of ‘invlpg’ makes pointer from integer without a cast [-Wint-conversion]
  229 |     invlpg(vaddr);
      |            ^~~~~
      |            |
      |            virt_addr_t {aka long unsigned int}
kernel/include/memory.h:144:33: note: expected ‘void *’ but argument is of type ‘virt_addr_t’ {aka ‘long unsigned int’}
  144 | static inline void invlpg(void *addr) {
      |                           ~~~~~~^~~~
  CC    kernel/timer.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
kernel/timer.c: In function ‘timer_irq_handler’:
kernel/timer.c:41:5: warning: implicit declaration of function ‘scheduler_tick’ [-Wimplicit-function-declaration]
   41 |     scheduler_tick();
      |     ^~~~~~~~~~~~~~
  CC    kernel/keyboard.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
  CC    kernel/serial.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
  CC    kernel/process.c
cc1: warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
kernel/process.c: In function ‘scheduler_init’:
kernel/process.c:40:27: warning: assignment to ‘uint64_t *’ {aka ‘long unsigned int *’} from incompatible pointer type ‘struct page_table *’ [-Wincompatible-pointer-types]
   40 |     idle_proc->page_table = &kernel_page_table;
      |                           ^
kernel/process.c: In function ‘process_create’:
kernel/process.c:70:22: warning: assignment to ‘uint64_t *’ {aka ‘long unsigned int *’} from incompatible pointer type ‘struct page_table *’ [-Wincompatible-pointer-types]
   70 |     proc->page_table = vmm_create_page_table();
      |                      ^
kernel/process.c:73:47: warning: passing argument 1 of ‘vmm_alloc_region’ from incompatible pointer type [-Wincompatible-pointer-types]
   73 |     proc->stack_bottom = vmm_alloc_region(proc->page_table, 16, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
      |                                           ~~~~^~~~~~~~~~~~
      |                                               |
      |                                               uint64_t * {aka long unsigned int *}
In file included from kernel/process.c:15:
kernel/include/memory.h:109:49: note: expected ‘struct page_table *’ but argument is of type ‘uint64_t *’ {aka ‘long unsigned int *’}
  109 | virt_addr_t vmm_alloc_region(struct page_table *pt, size_t pages, uint64_t flags);
      |                              ~~~~~~~~~~~~~~~~~~~^~
kernel/process.c:75:36: warning: passing argument 1 of ‘vmm_destroy_page_table’ from incompatible pointer type [-Wincompatible-pointer-types]
   75 |         vmm_destroy_page_table(proc->page_table);
      |                                ~~~~^~~~~~~~~~~~
      |                                    |
      |                                    uint64_t * {aka long unsigned int *}
kernel/include/memory.h:107:48: note: expected ‘struct page_table *’ but argument is of type ‘uint64_t *’ {aka ‘long unsigned int *’}
  107 | void vmm_destroy_page_table(struct page_table *pt);
      |                             ~~~~~~~~~~~~~~~~~~~^~
kernel/process.c: In function ‘scheduler_switch_task’:
kernel/process.c:160:35: warning: passing argument 1 of ‘vmm_switch_page_table’ from incompatible pointer type [-Wincompatible-pointer-types]
  160 |         vmm_switch_page_table(next->page_table);
      |                               ~~~~^~~~~~~~~~~~
      |                                   |
      |                                   uint64_t * {aka long unsigned int *}
kernel/include/memory.h:105:47: note: expected ‘struct page_table *’ but argument is of type ‘uint64_t *’ {aka ‘long unsigned int *’}
  105 | void vmm_switch_page_table(struct page_table *pt);
      |                            ~~~~~~~~~~~~~~~~~~~^~
kernel/process.c: In function ‘process_kill’:
kernel/process.c:185:46: warning: comparison of distinct pointer types lacks a cast
  185 |     if (proc->page_table && proc->page_table != &kernel_page_table)
      |                                              ^~
kernel/process.c:186:36: warning: passing argument 1 of ‘vmm_destroy_page_table’ from incompatible pointer type [-Wincompatible-pointer-types]
  186 |         vmm_destroy_page_table(proc->page_table);
      |                                ~~~~^~~~~~~~~~~~
      |                                    |
      |                                    uint64_t * {aka long unsigned int *}
kernel/include/memory.h:107:48: note: expected ‘struct page_table *’ but argument is of type ‘uint64_t *’ {aka ‘long unsigned int *’}
  107 | void vmm_destroy_page_table(struct page_table *pt);
      |                             ~~~~~~~~~~~~~~~~~~~^~
kernel/process.c: At top level:
kernel/process.c:328:14: error: static declaration of ‘strstr’ follows non-static declaration
  328 | static char *strstr(const char *haystack, const char *needle) {
      |              ^~~~~~
In file included from kernel/process.c:8:
/usr/include/string.h:350:14: note: previous declaration of ‘strstr’ with type ‘char *(const char *, const char *)’
  350 | extern char *strstr (const char *__haystack, const char *__needle)
      |              ^~~~~~
kernel/process.c:328:14: warning: ‘strstr’ defined but not used [-Wunused-function]
  328 | static char *strstr(const char *haystack, const char *needle) {
      |              ^~~~~~
make: *** [Makefile:92: build/process.o] Error 1
Process completed with exit code 2.