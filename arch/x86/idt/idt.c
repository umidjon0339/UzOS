/*
 * idt.c - Fill the Interrupt Descriptor Table and load it.
 *
 * The handler stubs live in arch/x86/isr/. Here we only wire
 * each vector number to its stub.
 */

#include "idt.h"
#include "gdt.h"
#include "isr.h"

// Gate flags: present, 32-bit interrupt gate (CPU clears IF on entry).
#define GATE_KERNEL 0x8E    // only ring 0 may 'int' this vector
#define GATE_USER   0xEE    // ring 3 may 'int' it too (for syscalls)

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

static void idt_set_gate(uint8_t vector, uint32_t handler, uint8_t flags) {
    idt[vector].base_lo = handler & 0xFFFF;
    idt[vector].base_hi = (handler >> 16) & 0xFFFF;
    idt[vector].sel     = GDT_KERNEL_CODE;
    idt[vector].always0 = 0;
    idt[vector].flags   = flags;
}

void init_idt(void) {
    // Unused vectors stay all-zero ("not present").

    for (int i = 0; i < ISR_EXCEPTIONS; i++)
        idt_set_gate(i, isr_stub_table[i], GATE_KERNEL);

    for (int i = 0; i < ISR_IRQS; i++)
        idt_set_gate(IRQ_BASE + i, irq_stub_table[i], GATE_KERNEL);

    idt_set_gate(SYSCALL_VECTOR, (uint32_t)isr128, GATE_USER);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    asm volatile("lidt %0" : : "m"(idt_ptr));
}
