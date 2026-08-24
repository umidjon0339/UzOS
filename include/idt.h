#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Initialize IDT, remap PIC, enable keyboard IRQ */
void idt_init(void);

#endif
