#include "mem_security.h"
#include "pmm.h"
#include "proc_security.h"

int mem_pmm_range_valid(const void *addr, size_t count) {
    if (!addr || !count) return 0;

    uint32_t address = (uint32_t)addr;
    if (address % PMM_BLOCK_SIZE) return 0;

    uint32_t frame = address / PMM_BLOCK_SIZE;
    uint32_t total = pmm_get_max_blocks();
    if (frame >= total) return 0;
    return count <= (size_t)(total - frame);
}

static int def_pmm_free_bounds_selftest(void) {
    uint32_t total = pmm_get_max_blocks();
    if (mem_pmm_range_valid(0, 1)) return 0;
    if (mem_pmm_range_valid((void *)1, 1)) return 0;
    if (mem_pmm_range_valid((void *)0xFFFFFFFFu, 1)) return 0;
    if (mem_pmm_range_valid((void *)0, 0)) return 0;
    if (total > 1) {
        uint32_t last = (total - 1) * PMM_BLOCK_SIZE;
        if (!mem_pmm_range_valid((void *)PMM_BLOCK_SIZE, 1)) return 0;
        if (!mem_pmm_range_valid((void *)last, 1)) return 0;
        if (mem_pmm_range_valid((void *)last, 2)) return 0;
    }
    return 1;
}

static const defence_t def = {
    "pmm_free_bounds",
    "reject unaligned or out-of-range physical frame releases",
    "pmm_free_oob",
    def_pmm_free_bounds_selftest
};

void def_pmm_free_bounds_register(void) {
    proc_security_register_defence(&def);
}
