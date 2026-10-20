/*
 * atk_user_ptr_wrap.c - ATTACK user_ptr_wrap: an address range that wraps.
 *
 * Breaks:     trust - addresses from a program must be checked first
 * Stopped by: user_ptr_bounds (security/defence/user_ptr_bounds)
 *
 * A program passes (start, size). The naive check "start + size <= END"
 * computes start + size in 32 bits, where 0xFFFFF000 + 0x2000 = 0x1000.
 * The range passes, yet it really covers the top of memory and the
 * kernel at the bottom. And a user address that is not mapped would make
 * the KERNEL page-fault while copying: the whole OS halts.
 *
 * simulate(): show what the naive check would let through.
 * launch():   offer the same ranges to the real checks.
 *
 * Uses: def_user_range_ok (security/defence/user_ptr_bounds).
 */

#include "security_lab.h"
#include "memory_layout.h"
#include "vga.h"
#include "def_user_ptr_bounds.h"

typedef struct {
    const char *what;
    uint32_t start, size;
} probe_t;

static const probe_t probes[] = {
    { "wraps past 4 GiB   ", 0xFFFFF000, 0x2000 },
    { "kernel code (1 MiB)", 0x00100000, 64     },
    { "kernel heap        ", KHEAP_START, 64    },
    { "runs off user space", USER_SPACE_END - 16, 64 },
    { "unmapped user page ", MMAP_END - PAGE_SIZE, 16 },
};
#define PROBE_COUNT (sizeof(probes) / sizeof(probes[0]))

// The check this attack beats: one addition, which can wrap.
static int naive_check(uint32_t start, uint32_t size) {
    return start + size <= USER_SPACE_END;
}

static void print_probe(const probe_t *p) {
    vga_print_str("  ");
    vga_print_str(p->what);
    vga_print_str(" ");
    vga_print_hex(p->start);
    vga_print_str(" + ");
    vga_print_hex(p->size);
}

static int simulate(void) {
    int fooled = 0;
    for (unsigned i = 0; i < PROBE_COUNT; i++) {
        int pass = naive_check(probes[i].start, probes[i].size);
        fooled += pass;
        print_probe(&probes[i]);
        vga_print_str(pass ? "  naive: PASSES\n" : "  naive: refused\n");
    }
    return fooled;
}

static int launch(void) {
    int refused = 0;
    for (unsigned i = 0; i < PROBE_COUNT; i++) {
        int ok = def_user_range_ok(probes[i].start, probes[i].size);
        refused += !ok;
        print_probe(&probes[i]);
        vga_print_str(ok ? "  -> ACCEPTED\n" : "  -> refused\n");
    }
    return refused;
}

static const attack_t attack = {
    "user_ptr_wrap",
    "an address range wraps past 4 GiB or points into the kernel",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // only checks were called: nothing to undo
    "user_ptr_bounds"
};

void atk_user_ptr_wrap_register(void) {
    proc_security_register_attack(&attack);
}
