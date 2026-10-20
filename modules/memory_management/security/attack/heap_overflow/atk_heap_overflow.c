/*
 * atk_heap_overflow.c - ATTACK heap_overflow: write past the end of a block.
 *
 * Breaks:     consistency - the heap's block list must stay correct
 * Stopped by: heap_canary (security/defence/heap_canary)
 *
 * Blocks sit right next to each other. Writing 4 bytes too many into
 * block A overwrites the magic number at the start of block B's header.
 *
 * simulate(): show where A ends and B's header begins.
 * launch():   with interrupts off: overflow A for real, kfree(A) (which
 *             walks the list and meets the broken header), then put B's
 *             header back exactly as it was. The heap is never left broken.
 *
 * Uses: kmalloc / kfree / kheap_get_stats (core/kheap), memset (lib),
 *       cpu_irq_save (arch/x86/cpu).
 */

#include "security_lab.h"
#include "kheap.h"
#include "string.h"
#include "cpu.h"
#include "vga.h"

#define BLOCK_SIZE 16
#define OVERFLOW   4    // just enough to hit the next header's magic

static int simulate(void) {
    vga_print_str("  block A data : 16 bytes\n");
    vga_print_str("  right after it: block B's header, starting with its magic\n");
    vga_print_str("  writing 20 bytes into A overwrites B's magic\n");
    return 1;
}

static int launch(void) {
    uint8_t *a = kmalloc(BLOCK_SIZE);
    uint8_t *b = kmalloc(BLOCK_SIZE);
    if (!a || !b)
        return 0;

    kheap_header_t *b_header = (kheap_header_t *)(b - sizeof(kheap_header_t));
    if ((uint8_t *)b_header != a + BLOCK_SIZE) {
        vga_print_str("  blocks are not neighbours this time; nothing done\n");
        kfree(a);
        kfree(b);
        return 0;
    }

    uint32_t denied = sec_total_denied();
    uint32_t irq = cpu_irq_save();          // nobody else may walk the heap now

    kheap_header_t saved = *b_header;
    memset(a, 'A', BLOCK_SIZE + OVERFLOW);  // the overflow: B's magic = "AAAA"

    kheap_stats_t s;
    kheap_get_stats(&s);
    vga_print_str(s.corrupted ? "  B's header is overwritten (heapinfo: CORRUPTED)\n"
                              : "  overflow missed B's header\n");

    kfree(a);                               // walks the list, meets B
    *b_header = saved;                      // repair B exactly

    cpu_irq_restore(irq);

    kfree(b);
    kheap_get_stats(&s);
    vga_print_str(s.corrupted ? "  heap still CORRUPTED\n" : "  B repaired, heap healthy\n");
    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "heap_overflow",
    "writing past a block's end overwrites the next block's header",
    INV_CONSISTENCY, SEV_CRITICAL,
    simulate, launch, 0,        // launch repairs the header itself
    "heap_canary"
};

void atk_heap_overflow_register(void) {
    proc_security_register_attack(&attack);
}
