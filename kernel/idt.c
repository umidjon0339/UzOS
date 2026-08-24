#include "idt.h"
#include "io.h"

/* IDT entry (64-bit long mode format) */
struct idt_entry {
    uint16_t offset_low;   /* offset bits 0-15 */
    uint16_t selector;     /* code segment selector in GDT */
    uint8_t  ist;          /* interrupt stack table (0 = not used) */
    uint8_t  type_attr;    /* type and attributes */
    uint16_t offset_mid;   /* offset bits 16-31 */
    uint32_t offset_high;  /* offset bits 32-63 */
    uint32_t zero;         /* reserved */
} __attribute__((packed));

/* IDT pointer for lidt instruction */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* 256 IDT entries */
static struct idt_entry idt[256];
static struct idt_ptr idtp;

/* ASM stub for keyboard IRQ (defined in isr.asm) */
extern void irq1_handler(void);

/* Set one IDT gate */
static void idt_set_gate(uint8_t num, uint64_t handler) {
    idt[num].offset_low  = handler & 0xFFFF;
    idt[num].selector    = 0x08;          /* kernel code segment */
    idt[num].ist         = 0;
    idt[num].type_attr   = 0x8E;          /* present, ring 0, 64-bit interrupt gate */
    idt[num].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].zero        = 0;
}

/* Remap PIC so IRQ 0-15 map to vectors 32-47 (avoid conflict with CPU exceptions 0-31) */
static void pic_remap(void) {
    /* ICW1: begin init, expect ICW4 */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    /* ICW2: vector offsets */
    outb(0x21, 0x20);  /* master: IRQ 0-7 -> vectors 32-39 */
    outb(0xA1, 0x28);  /* slave:  IRQ 8-15 -> vectors 40-47 */
    /* ICW3: master/slave wiring */
    outb(0x21, 0x04);  /* master: slave on IRQ2 */
    outb(0xA1, 0x02);  /* slave:  cascade identity */
    /* ICW4: 8086 mode */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    /* Mask all IRQs except IRQ1 (keyboard) */
    outb(0x21, 0xFD);  /* master: bit1=0 means IRQ1 enabled */
    outb(0xA1, 0xFF);  /* slave: all masked */
}

void idt_init(void) {
    /* Zero out all entries */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0);
    }

    /* Remap PIC before setting IRQ gates */
    pic_remap();

    /* IRQ1 (keyboard) is vector 33 after PIC remap */
    idt_set_gate(33, (uint64_t)irq1_handler);

    /* Load IDT */
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;
    __asm__ volatile ("lidt %0" : : "m"(idtp));

    /* Enable interrupts */
    __asm__ volatile ("sti");
}
