/*
 * atk_heap_double_free.c - ATTACK heap_double_free: kfree() what isn't yours.
 *
 * Breaks:     consistency - the heap's block list must stay correct
 * Stopped by: heap_guard (security/defence/heap_guard)
 *
 * kfree(p) writes into the header just before p. With a pointer from
 * outside the heap, from the middle of a block, or one that was already
 * freed, that write hits the wrong memory.
 *
 * simulate(): list the bad frees.
 * launch():   really call kfree() with each one, then check heapinfo is
 *             still healthy.
 *
 * Uses: kmalloc / kfree / kheap_get_stats (core/kheap).
 */

#include "registry.h"
#include "audit.h"
#include "kheap.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  kfree the same block twice\n");
    vga_print_str("  kfree a pointer into the middle of a block\n");
    vga_print_str("  kfree a stack address (not heap memory)\n");
    return 3;
}

static void try_free(const char *what, void *p) {
    uint32_t denied = audit_total();
    kfree(p);
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(audit_total() > denied ? " -> refused\n" : " -> FREED\n");
}

static int launch(void) {
    uint32_t denied = audit_total();
    int on_stack = 0;

    uint8_t *a = kmalloc(32);
    uint8_t *b = kmalloc(64);
    if (!a || !b)
        return 0;

    kfree(a);                                   // the one legal free
    try_free("second kfree of a  ", a);
    try_free("middle of block b  ", b + 8);
    try_free("stack address      ", &on_stack);
    kfree(b);                                   // legal: clean up

    kheap_stats_t s;
    kheap_get_stats(&s);
    vga_print_str(s.corrupted ? "  heap: CORRUPTED\n" : "  heap: still healthy\n");
    return audit_total() - denied;
}

static const attack_t attack = {
    "heap_double_free",
    "kfree a block twice, a pointer into a block, or a non-heap address",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, 0,        // the blocks it used are freed again
    "heap_guard"
};

void atk_heap_double_free_register(void) {
    registry_add_attack(&attack);
}
