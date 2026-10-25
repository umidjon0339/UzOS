/*
 * def_irq_mask.c - DEFENCE irq_mask: only lines with a driver are open.
 *
 * Stops:  irq_storm (security/attack/irq_storm)
 * Called: pic_unmask() (pic/), before a line is opened.
 * Rule:   irq < 16, and it is the cascade line or a handler is
 *         registered for its vector.
 * Why:    an open line with no driver can still fire. Nobody reads the
 *         device, so it keeps asking: each time the CPU stops, finds no
 *         handler, sends EOI, and gets interrupted again. A noisy or
 *         broken device can keep the CPU busy forever (an "interrupt
 *         storm"). Lines are closed until a driver is ready for them.
 */

#include "def_irq_mask.h"
#include "registry.h"
#include "audit.h"
#include "pic.h"
#include "isr.h"

// The whole rule, as a pure function so the selftest can try it.
static int may_unmask(uint8_t irq, int has_handler) {
    if (irq >= 16) return 0;
    return irq == PIC_CASCADE_IRQ || has_handler;
}

int def_irq_unmask_ok(uint8_t irq) {
    if (may_unmask(irq, irq < 16 && isr_has_handler(IRQ_BASE + irq)))
        return 1;
    audit_log("irq_mask", "IRQ line kept masked: no driver handles it");
    return 0;
}

static int selftest(void) {
    if (!may_unmask(0, 1)) return 0;                // timer with its handler
    if (may_unmask(3, 0)) return 0;                 // serial port, no driver
    if (!may_unmask(PIC_CASCADE_IRQ, 0)) return 0;  // cascade needs no handler
    if (may_unmask(16, 1)) return 0;                // there is no IRQ 16
    return 1;
}

static const defence_t defence = {
    "irq_mask",
    "keep every IRQ line masked until a driver handles it",
    "irq_storm",
    selftest
};

void def_irq_mask_register(void) {
    registry_add_defence(&defence);
}
