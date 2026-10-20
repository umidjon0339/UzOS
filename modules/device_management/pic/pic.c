/*
 * pic.c - Program the two 8259 PICs.
 *
 * Uses: outb/inb/io_wait (arch/x86/io), and the defence irq_mask for
 *       every unmask.
 */

#include "pic.h"
#include "io.h"
#include "def_irq_mask.h"

// Initialisation Command Words
#define ICW1_INIT  0x10         // start initialisation
#define ICW1_ICW4  0x01         // ICW4 will follow
#define ICW4_8086  0x01         // 8086 mode

#define ALL_MASKED 0xFF

void pic_remap(int offset1, int offset2) {
    // ICW1: start the 4-step setup on both chips.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);  io_wait();

    // ICW2: the first vector of each chip.
    outb(PIC1_DATA, offset1);                   io_wait();
    outb(PIC2_DATA, offset2);                   io_wait();

    // ICW3: master has the slave on IRQ 2; slave's cascade identity is 2.
    outb(PIC1_DATA, 1 << PIC_CASCADE_IRQ);      io_wait();
    outb(PIC2_DATA, PIC_CASCADE_IRQ);           io_wait();

    // ICW4: 8086 mode.
    outb(PIC1_DATA, ICW4_8086);                 io_wait();
    outb(PIC2_DATA, ICW4_8086);                 io_wait();

    // Block everything except the cascade. A line opens when its driver
    // is ready (pic_unmask), so no IRQ ever arrives without a handler.
    outb(PIC1_DATA, ALL_MASKED & ~(1 << PIC_CASCADE_IRQ));
    outb(PIC2_DATA, ALL_MASKED);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);    // slave first, then the master
    outb(PIC1_COMMAND, PIC_EOI);
}

// Data port and bit for an IRQ line.
static uint16_t mask_port(uint8_t irq) { return irq < 8 ? PIC1_DATA : PIC2_DATA; }
static uint8_t  mask_bit(uint8_t irq)  { return 1 << (irq % 8); }

int pic_unmask(uint8_t irq) {
    // DEFENCE irq_mask: only open a line some driver handles.
    if (!def_irq_unmask_ok(irq))
        return 0;
    outb(mask_port(irq), inb(mask_port(irq)) & ~mask_bit(irq));
    return 1;
}

void pic_mask(uint8_t irq) {
    if (irq < 16)
        outb(mask_port(irq), inb(mask_port(irq)) | mask_bit(irq));
}

uint16_t pic_get_mask(void) {
    return inb(PIC1_DATA) | (inb(PIC2_DATA) << 8);
}
