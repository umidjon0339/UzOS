/*
 * atk_munmap_kernel.c - ATTACK munmap_kernel: unmap memory you never mapped.
 *
 * Breaks:     isolation - you can only give back what you were given
 * Stopped by: mmap_ownership (security/defence/mmap_ownership)
 *
 * munmap() frees the frames behind the pages. If it accepted any
 * address, munmap(<kernel address>) would free the kernel's own frames.
 *
 * simulate(): list the targets.
 * launch():   really call sys_munmap() on each; the used-frame count must
 *             not change. Then a real mmap/munmap pair must still work.
 *
 * Uses: sys_mmap / sys_munmap (syscalls/), pmm_get_used_blocks (core/pmm).
 */

#include "registry.h"
#include "audit.h"
#include "memory_layout.h"
#include "pmm.h"
#include "vga.h"
#include "sys_mmap.h"
#include "sys_munmap.h"

static int simulate(void) {
    vga_print_str("  munmap the kernel's code  0x00100000\n");
    vga_print_str("  munmap the kernel heap    0x01000000\n");
    vga_print_str("  munmap half of an mmap() region\n");
    return 3;
}

static void try_munmap(const char *what, void *addr, uint32_t length) {
    uint32_t used = pmm_get_used_blocks();
    int result = sys_munmap(addr, length);
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(result == 0 ? " -> UNMAPPED" : " -> refused");
    vga_print_str(pmm_get_used_blocks() == used ? ", frames untouched\n" : ", frames FREED\n");
}

static int launch(void) {
    uint32_t denied = audit_total();

    try_munmap("kernel code ", (void *)0x00100000, PAGE_SIZE);
    try_munmap("kernel heap ", (void *)KHEAP_START, PAGE_SIZE);

    void *region = sys_mmap(2 * PAGE_SIZE);
    if (region) {
        try_munmap("half region ", region, PAGE_SIZE);
        vga_print_str(sys_munmap(region, 2 * PAGE_SIZE) == 0
                      ? "  whole region -> unmapped (allowed, it is ours)\n"
                      : "  whole region -> FAILED\n");
    }
    return audit_total() - denied;
}

static const attack_t attack = {
    "munmap_kernel",
    "munmap kernel memory so its frames get freed and reused",
    INV_ISOLATION, SEV_CRITICAL,
    simulate, launch, 0,        // the one region it mapped is unmapped again
    "mmap_ownership"
};

void atk_munmap_kernel_register(void) {
    registry_add_attack(&attack);
}
