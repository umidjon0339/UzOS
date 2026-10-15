#include "mem_security.h"
#include "pmm.h"
#include "proc_security.h"
#include "vga.h"

static int atk_pmm_free_oob_check(void) {
    uint32_t total = pmm_get_max_blocks();
    uint32_t out_of_range = total < 1048576u
        ? total * PMM_BLOCK_SIZE
        : (total - 1) * PMM_BLOCK_SIZE;
    size_t out_of_range_count = total < 1048576u ? 1 : 2;

    int refused = !mem_pmm_range_valid((void *)1, 1);
    if (!mem_pmm_range_valid((void *)out_of_range, out_of_range_count))
        refused++;
    return refused;
}

static int atk_pmm_free_oob_simulate(void) {
    vga_print_str("  probes target misaligned and out-of-range frame addresses\n");
    vga_print_str("  no physical frame is released\n");
    return atk_pmm_free_oob_check();
}

static int atk_pmm_free_oob_launch(void) {
    uint32_t used_before = pmm_get_used_blocks();
    uint32_t total = pmm_get_max_blocks();
    uint32_t out_of_range = total < 1048576u
        ? total * PMM_BLOCK_SIZE
        : (total - 1) * PMM_BLOCK_SIZE;
    size_t out_of_range_count = total < 1048576u ? 1 : 2;

    pmm_free_blocks((void *)out_of_range, out_of_range_count);
    int unchanged = pmm_get_used_blocks() == used_before;

    vga_print_str("  attempted an out-of-range release; used frames ");
    vga_print_str(unchanged ? "unchanged\n" : "changed\n");
    return unchanged ? atk_pmm_free_oob_check() : 0;
}

static const attack_t atk = {
    "pmm_free_oob",
    "an invalid physical address can index beyond the PMM bitmap",
    INV_CONSISTENCY,
    SEV_HIGH,
    atk_pmm_free_oob_simulate,
    atk_pmm_free_oob_launch,
    0,
    "pmm_free_bounds"
};

void atk_pmm_free_oob_register(void) {
    proc_security_register_attack(&atk);
}
