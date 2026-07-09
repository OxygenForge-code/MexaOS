/* ============================================================
 *  MexaOS Interrupt Handling
 *  IDT setup, ISR/IRQ handlers with MexaOS themed output
 * ============================================================ */

#include "../../include/mexaos.h"
#include "include/interrupt.h"
#include "include/vga.h"

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_descriptor idt_desc;
static void (*irq_handlers[16])(struct interrupt_frame *);
static uint64_t irq_counts[16] = {0};

/* ─── Initialize IDT ─── */
void idt_init(void) {
    /* Zero all entries */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].reserved = 0;
    }
    
    /* Set up ISR gates (0-31: CPU exceptions) */
    idt_set_gate(0,  (uint64_t)isr0,  0x8E, 0);
    idt_set_gate(1,  (uint64_t)isr1,  0x8E, 0);
    idt_set_gate(2,  (uint64_t)isr2,  0x8E, 0);
    idt_set_gate(3,  (uint64_t)isr3,  0x8E, 0);
    idt_set_gate(4,  (uint64_t)isr4,  0x8E, 0);
    idt_set_gate(5,  (uint64_t)isr5,  0x8E, 0);
    idt_set_gate(6,  (uint64_t)isr6,  0x8E, 0);
    idt_set_gate(7,  (uint64_t)isr7,  0x8E, 0);
    idt_set_gate(8,  (uint64_t)isr8,  0x8E, 0);
    idt_set_gate(9,  (uint64_t)isr9,  0x8E, 0);
    idt_set_gate(10, (uint64_t)isr10, 0x8E, 0);
    idt_set_gate(11, (uint64_t)isr11, 0x8E, 0);
    idt_set_gate(12, (uint64_t)isr12, 0x8E, 0);
    idt_set_gate(13, (uint64_t)isr13, 0x8E, 0);
    idt_set_gate(14, (uint64_t)isr14, 0x8E, 0);
    idt_set_gate(15, (uint64_t)isr15, 0x8E, 0);
    idt_set_gate(16, (uint64_t)isr16, 0x8E, 0);
    idt_set_gate(17, (uint64_t)isr17, 0x8E, 0);
    idt_set_gate(18, (uint64_t)isr18, 0x8E, 0);
    idt_set_gate(19, (uint64_t)isr19, 0x8E, 0);
    idt_set_gate(20, (uint64_t)isr20, 0x8E, 0);
    idt_set_gate(21, (uint64_t)isr21, 0x8E, 0);
    idt_set_gate(22, (uint64_t)isr22, 0x8E, 0);
    idt_set_gate(23, (uint64_t)isr23, 0x8E, 0);
    idt_set_gate(24, (uint64_t)isr24, 0x8E, 0);
    idt_set_gate(25, (uint64_t)isr25, 0x8E, 0);
    idt_set_gate(26, (uint64_t)isr26, 0x8E, 0);
    idt_set_gate(27, (uint64_t)isr27, 0x8E, 0);
    idt_set_gate(28, (uint64_t)isr28, 0x8E, 0);
    idt_set_gate(29, (uint64_t)isr29, 0x8E, 0);
    idt_set_gate(30, (uint64_t)isr30, 0x8E, 0);
    idt_set_gate(31, (uint64_t)isr31, 0x8E, 0);
    
    /* Set up IRQ gates (remapped to 0x20-0x2F) */
    idt_set_gate(IRQ0_VECTOR + 0,  (uint64_t)irq0,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 1,  (uint64_t)irq1,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 2,  (uint64_t)irq2,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 3,  (uint64_t)irq3,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 4,  (uint64_t)irq4,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 5,  (uint64_t)irq5,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 6,  (uint64_t)irq6,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 7,  (uint64_t)irq7,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 8,  (uint64_t)irq8,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 9,  (uint64_t)irq9,  0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 10, (uint64_t)irq10, 0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 11, (uint64_t)irq11, 0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 12, (uint64_t)irq12, 0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 13, (uint64_t)irq13, 0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 14, (uint64_t)irq14, 0x8E, 0);
    idt_set_gate(IRQ0_VECTOR + 15, (uint64_t)irq15, 0x8E, 0);
    
    /* Syscall gate (int 0x80) */
    idt_set_gate(INT_SYSCALL, (uint64_t)isr128, 0xEE, 3);
    
    /* Load IDT */
    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base = (uint64_t)&idt;
    idt_load(&idt_desc);
}

/* ─── Set IDT Gate ─── */
void idt_set_gate(uint8_t num, uint64_t handler, uint8_t type, uint8_t dpl) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = 0x08;
    idt[num].ist = 0;
    idt[num].type_attr = type | (dpl << 5);
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].reserved = 0;
}

/* ─── Load IDT ─── */
void idt_load(struct idt_descriptor *desc) {
    __asm__ __volatile__("lidt %0" :: "m"(*desc));
}

/* ─── Initialize PIC ─── */
void pic_init(void) {
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);
    outb(PIC1_DATA, IRQ0_VECTOR);
    outb(PIC2_DATA, IRQ8_VECTOR);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* ─── PIC Control ─── */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port) | (1 << (irq & 7));
    outb(port, mask);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port) & ~(1 << (irq & 7));
    outb(port, mask);
}

/* ─── ISR Handler ─── */
void isr_handler(struct interrupt_frame *frame) {
    uint64_t int_num = frame->int_num;
    
    if (int_num < 32) {
        cli();
        
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_puts("\n╔══════════════════════════════════════════════════════════════╗\n");
        vga_puts("║  ✦ MexaOS Exception — System Halted                        ║\n");
        vga_puts("╠══════════════════════════════════════════════════════════════╣\n");
        
        vga_puts("║  Exception #");
        vga_print_dec(int_num);
        vga_puts(": ");
        
        if (int_num < 32 && exception_names[int_num]) {
            vga_puts(exception_names[int_num]);
        } else {
            vga_puts("Unknown");
        }
        vga_puts("\n");
        
        if (frame->error_code != 0) {
            vga_puts("║  Error Code: 0x");
            vga_print_hex(frame->error_code);
            vga_puts("\n");
        }
        
        if (int_num == 14) {
            uint64_t cr2;
            __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
            vga_puts("║  Fault Address (CR2): 0x");
            vga_print_hex(cr2);
            vga_puts("\n");
        }
        
        vga_puts("╠══════════════════════════════════════════════════════════════╣\n");
        vga_puts("║  Register Dump:                                              ║\n");
        vga_puts("║  RAX="); vga_print_hex(frame->rax);
        vga_puts(" RBX="); vga_print_hex(frame->rbx);
        vga_puts(" RCX="); vga_print_hex(frame->rcx);
        vga_puts(" RDX="); vga_print_hex(frame->rdx);
        vga_puts("\n");
        vga_puts("║  RBP="); vga_print_hex(frame->rbp);
        vga_puts(" RIP="); vga_print_hex(frame->rip);
        vga_puts(" RSP="); vga_print_hex(frame->rsp);
        vga_puts("\n");
        vga_puts("╚══════════════════════════════════════════════════════════════╝\n");
        
        while (1) hlt();
    }
}

/* ─── IRQ Handler ─── */
void irq_handler(struct interrupt_frame *frame) {
    uint8_t irq = frame->int_num - IRQ0_VECTOR;
    irq_counts[irq]++;
    
    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](frame);
    }
    
    pic_send_eoi(irq);
}

/* ─── Register IRQ Handler ─── */
void irq_register_handler(uint8_t irq, void (*handler)(struct interrupt_frame *)) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

/* ─── Get IRQ Count ─── */
uint64_t irq_get_count(uint8_t irq) {
    return (irq < 16) ? irq_counts[irq] : 0;
}
