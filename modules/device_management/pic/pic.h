#ifndef PIC_H
#define PIC_H

/*
 * pic.h - 8259 Programmable Interrupt Controller.
 *
 * Devices raise an IRQ line (0-15). Two chained PICs turn that into a CPU
 * interrupt vector. The master handles IRQ 0-7, the slave IRQ 8-15 and
 * reaches the CPU through the master's IRQ 2 (the "cascade").
 *
 *   IRQ 0 timer   IRQ 1 keyboard   IRQ 2 cascade   IRQ 12 mouse
 *
 * After each interrupt the CPU must send an EOI ("end of interrupt"),
 * or the PIC will not deliver that line again.
 */

#include <stdint.h>

#define PIC1_COMMAND 0x20       // master
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0       // slave
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20       // "end of interrupt" command
#define PIC_CASCADE_IRQ 2

// Move IRQ 0-15 to vectors offset1.. and offset2.., and block every
// line except the cascade. Drivers unmask their own line.
void pic_remap(int offset1, int offset2);

// Tell the PIC(s) the interrupt for 'irq' has been handled.
void pic_send_eoi(uint8_t irq);

// Let 'irq' through. Refused (returns 0) if no driver handles it (irq_mask).
int pic_unmask(uint8_t irq);

// Block 'irq' again.
void pic_mask(uint8_t irq);

// Bit n set = IRQ n is blocked.
uint16_t pic_get_mask(void);

#endif
