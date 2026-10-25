/*
 * atk_irq_storm.c - ATTACK irq_storm: open IRQ lines nobody handles.
 *
 * Breaks:     liveness - no device may keep the CPU busy forever
 * Stopped by: irq_mask (security/defence/irq_mask)
 *
 * A device on an open line with no driver keeps asking for attention:
 * the CPU is interrupted, finds no handler, sends EOI, and is interrupted
 * again. The old pic_remap() opened all 16 lines.
 *
 * simulate(): list every line: open or masked, handler or not.
 * launch():   try to open the serial and printer lines, which have no
 *             driver; the PIC mask must not change.
 *
 * Uses: pic_unmask / pic_get_mask (pic/), isr_has_handler (arch/x86/isr).
 */

#include "registry.h"
#include "audit.h"
#include "pic.h"
#include "isr.h"
#include "vga.h"

static const struct { uint8_t irq; const char *name; } targets[] = {
    { 3, "COM2 serial" }, { 4, "COM1 serial" }, { 5, "LPT2 / sound" }, { 7, "LPT1 printer" },
};
#define TARGET_COUNT (sizeof(targets) / sizeof(targets[0]))

static int simulate(void) {
    uint16_t mask = pic_get_mask();
    int risky = 0;

    vga_print_str("  open lines:");
    for (uint8_t irq = 0; irq < 16; irq++) {
        if (mask & (1 << irq))
            continue;
        vga_print_str(" ");
        vga_print_int(irq);
        if (irq != PIC_CASCADE_IRQ && !isr_has_handler(IRQ_BASE + irq)) {
            vga_print_str("(no handler!)");
            risky++;
        }
    }
    vga_print_str("\n  open lines without a handler: ");
    vga_print_int(risky);
    vga_print_str("\n");
    return risky;
}

static int launch(void) {
    uint32_t denied = audit_total();
    uint16_t before = pic_get_mask();

    for (unsigned i = 0; i < TARGET_COUNT; i++) {
        int opened = pic_unmask(targets[i].irq);
        vga_print_str("  unmask IRQ ");
        vga_print_int(targets[i].irq);
        vga_print_str(" (");
        vga_print_str(targets[i].name);
        vga_print_str(opened ? ") -> OPENED\n" : ") -> refused\n");
        if (opened)
            pic_mask(targets[i].irq);       // close it again
    }

    vga_print_str(pic_get_mask() == before ? "  PIC mask unchanged\n" : "  PIC mask CHANGED\n");
    return audit_total() - denied;
}

static const attack_t attack = {
    "irq_storm",
    "open IRQ lines that no driver handles, inviting an interrupt storm",
    INV_LIVENESS, SEV_MEDIUM,
    simulate, launch, 0,        // any line it opened is masked again
    "irq_mask"
};

void atk_irq_storm_register(void) {
    registry_add_attack(&attack);
}
