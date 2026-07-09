/* ============================================================
 *  MexaOS Interrupt Handling Header
 *  IDT, ISR, IRQ — MexaOS themed exception messages
 * ============================================================ */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "../../include/mexaos.h"

/* ─── Interrupt Frame ─── */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_num, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

/* ─── IDT Entry ─── */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

/* ─── IDT Descriptor ─── */
struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* ─── Functions ─── */
void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t handler, uint8_t type, uint8_t dpl);
void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void idt_load(struct idt_descriptor *desc);

/* ─── ISR Handlers ─── */
void isr_handler(struct interrupt_frame *frame);
void irq_handler(struct interrupt_frame *frame);

/* ─── Exception Messages (MexaOS themed) ─── */
static const char *exception_names[] __attribute__((unused)) = {
    "Divide by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

/* ─── ISR Stubs (Assembly) ─── */
extern void isr0(void);   extern void isr1(void);
extern void isr2(void);   extern void isr3(void);
extern void isr4(void);   extern void isr5(void);
extern void isr6(void);   extern void isr7(void);
extern void isr8(void);   extern void isr9(void);
extern void isr10(void);  extern void isr11(void);
extern void isr12(void);  extern void isr13(void);
extern void isr14(void);  extern void isr15(void);
extern void isr16(void);  extern void isr17(void);
extern void isr18(void);  extern void isr19(void);
extern void isr20(void);  extern void isr21(void);
extern void isr22(void);  extern void isr23(void);
extern void isr24(void);  extern void isr25(void);
extern void isr26(void);  extern void isr27(void);
extern void isr28(void);  extern void isr29(void);
extern void isr30(void);  extern void isr31(void);

/* ─── IRQ Stubs ─── */
extern void irq0(void);   extern void irq1(void);
extern void irq2(void);   extern void irq3(void);
extern void irq4(void);   extern void irq5(void);
extern void irq6(void);   extern void irq7(void);
extern void irq8(void);   extern void irq9(void);
extern void irq10(void);  extern void irq11(void);
extern void irq12(void);  extern void irq13(void);
extern void irq14(void);  extern void irq15(void);

/* ─── Syscall ─── */
extern void isr128(void);

#endif /* INTERRUPT_H */
