#ifndef DEF_IRQ_MASK_H
#define DEF_IRQ_MASK_H

#include <stdint.h>

// May IRQ line 'irq' be unmasked? 1 = yes, 0 = refused (logged).
int def_irq_unmask_ok(uint8_t irq);

#endif
