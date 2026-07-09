/* ============================================================
 *  MexaOS Kernel Linker Script
 *  x86_64 higher half kernel layout
 * ============================================================ */

OUTPUT_FORMAT("elf64-x86-64")
ENTRY(kmain)

KERNEL_VIRT = 0xFFFFFFFF80000000;

SECTIONS
{
    . = 0x100000;
    
    .multiboot ALIGN(8) :
    {
        *(.multiboot)
    }
    
    .boot ALIGN(4K) :
    {
        *(.boot)
        build/boot.o(.text)
        build/stage2.o(.text)
    }
    
    . += KERNEL_VIRT;
    
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT)
    {
        _code = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4K);
        _code_end = .;
    }
    
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT)
    {
        _data = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4K);
        _data_end = .;
    }
    
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT)
    {
        _bss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4K);
        _bss_end = .;
    }
    
    _kernel_end = .;
    
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.*)
        *(.eh_frame)
        *(.gnu.*)
    }
}
