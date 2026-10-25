/*
 * atk_pmm_bad_free.c - ATTACK pmm_bad_free: free frames you don't own.
 *
 * Breaks:     consistency - each frame has exactly one owner
 * Stopped by: pmm_free_guard (security/defence/pmm_free_guard)
 *
 * A freed frame goes to the next pmm_alloc_block() caller. So freeing a
 * kernel frame, or freeing a frame twice, makes two owners write to the
 * same RAM.
 *
 * simulate(): list the bad frees that would be tried.
 * launch():   really call pmm_free_block() with each one; the used-frame
 *             count must not change.
 *
 * Uses: pmm_alloc_block / pmm_free_block / pmm_get_used_blocks (core/pmm),
 *       _kernel_start (boot/linker.ld).
 */

#include "registry.h"
#include "audit.h"
#include "pmm.h"
#include "vga.h"

extern uint8_t _kernel_start[];

static int simulate(void) {
    vga_print_str("  free the kernel's own frame   ");
    vga_print_hex((uint32_t)_kernel_start);
    vga_print_str("\n  free a misaligned address     0x00200001\n");
    vga_print_str("  free past the end of RAM\n");
    vga_print_str("  free the same frame twice\n");
    return 4;
}

static void try_free(const char *what, uint32_t addr) {
    uint32_t used_before = pmm_get_used_blocks();
    pmm_free_block((void *)addr);
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(pmm_get_used_blocks() == used_before ? " -> refused\n" : " -> FREED\n");
}

static int launch(void) {
    uint32_t denied = audit_total();

    try_free("kernel frame      ", (uint32_t)_kernel_start);
    try_free("misaligned address", 0x00200001);
    try_free("past the end of RAM", pmm_get_max_blocks() * PAGE_SIZE);

    // A real double free: take a frame, free it, free it again.
    void *frame = pmm_alloc_block();
    if (frame) {
        pmm_free_block(frame);
        try_free("second free (double)", (uint32_t)frame);
    }

    return audit_total() - denied;
}

static const attack_t attack = {
    "pmm_bad_free",
    "free a kernel frame, a bogus address, or a frame twice",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, 0,        // refused frees change nothing
    "pmm_free_guard"
};

void atk_pmm_bad_free_register(void) {
    registry_add_attack(&attack);
}
