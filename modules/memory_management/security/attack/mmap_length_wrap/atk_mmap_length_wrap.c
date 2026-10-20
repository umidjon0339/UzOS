/*
 * atk_mmap_length_wrap.c - ATTACK mmap_length_wrap: a length that rounds to 0.
 *
 * Breaks:     resource - every mapped byte needs a real page behind it
 * Stopped by: mmap_page_count (security/defence/mmap_page_count)
 *
 * The usual rounding (length + 4095) / 4096 overflows for a length near
 * 4 GiB: 0xFFFFFFFF + 4095 wraps to 0xFFE, which is 0 pages. mmap would
 * then "succeed" with nothing behind the address.
 *
 * simulate(): show what the naive formula gives.
 * launch():   call the real sys_mmap() with bad lengths (must fail) and
 *             one good length (must work).
 *
 * Uses: sys_mmap / sys_munmap (syscalls/).
 */

#include "security_lab.h"
#include "memory_layout.h"
#include "vga.h"
#include "sys_mmap.h"
#include "sys_munmap.h"
#include "def_mmap_page_count.h"

static const uint32_t bad_lengths[] = { 0xFFFFFFFF, 0, MMAP_MAX_BYTES + 1 };
#define BAD_COUNT (sizeof(bad_lengths) / sizeof(bad_lengths[0]))

// The formula this attack beats.
static uint32_t naive_pages(uint32_t length) {
    return (length + PAGE_SIZE - 1) / PAGE_SIZE;
}

static int simulate(void) {
    vga_print_str("  naive pages for length 0xFFFFFFFF: ");
    vga_print_int(naive_pages(0xFFFFFFFF));
    vga_print_str("  (should be ~1 million)\n");
    return naive_pages(0xFFFFFFFF) == 0;
}

static int launch(void) {
    int refused = 0;

    for (unsigned i = 0; i < BAD_COUNT; i++) {
        void *p = sys_mmap(bad_lengths[i]);
        refused += (p == 0);
        vga_print_str("  mmap(");
        vga_print_hex(bad_lengths[i]);
        vga_print_str(p ? ") -> MAPPED\n" : ") -> refused\n");
        if (p)
            sys_munmap(p, bad_lengths[i]);
    }

    void *ok = sys_mmap(PAGE_SIZE);     // a normal request must still work
    vga_print_str(ok ? "  mmap(4096) -> works\n" : "  mmap(4096) -> FAILED\n");
    if (ok)
        sys_munmap(ok, PAGE_SIZE);
    return refused;
}

static const attack_t attack = {
    "mmap_length_wrap",
    "a huge mmap length rounds to 0 pages and maps nothing",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, 0,        // every successful mmap is unmapped again
    "mmap_page_count"
};

void atk_mmap_length_wrap_register(void) {
    proc_security_register_attack(&attack);
}
