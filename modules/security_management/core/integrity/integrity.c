/*
 * integrity.c - Measure the kernel at boot, and again when asked.
 *
 * Where the regions are comes from the linker (boot/linker.ld): it puts
 * a symbol at the start and end of .text and .rodata. A symbol is only
 * an address, so we declare them as arrays and never read them as
 * values.
 *
 * Uses: sha256 (user_management/core/hash), def_integrity_unchanged
 *       (security/defence/integrity_check).
 */

#include "integrity.h"
#include "def_integrity_check.h"

extern const uint8_t _text_start[], _text_end[];
extern const uint8_t _rodata_start[], _rodata_end[];

static integrity_region_t regions[INTEGRITY_REGIONS] = {
    { "code",      ".text",   "kernel code (.text) changed since boot",
      _text_start, _text_end, {0} },
    { "constants", ".rodata", "kernel constants (.rodata) changed since boot",
      _rodata_start, _rodata_end, {0} },
};

static void measure(const integrity_region_t *r, uint8_t out[SHA256_SIZE]) {
    sha256(r->start, (uint32_t)(r->end - r->start), out);
}

void integrity_init(void) {
    for (int i = 0; i < INTEGRITY_REGIONS; i++)
        measure(&regions[i], regions[i].at_boot);
}

int integrity_region_count(void) { return INTEGRITY_REGIONS; }

const integrity_region_t *integrity_region(int index) {
    if (index < 0 || index >= INTEGRITY_REGIONS) return 0;
    return &regions[index];
}

int integrity_verify_region(int index, uint8_t now[SHA256_SIZE]) {
    const integrity_region_t *r = integrity_region(index);
    if (!r)
        return 0;
    measure(r, now);
    return def_integrity_unchanged(r->alarm, r->at_boot, now);   // DEFENCE
}

int integrity_verify(void) {
    uint8_t now[SHA256_SIZE];
    int unchanged = 1;
    for (int i = 0; i < INTEGRITY_REGIONS; i++)
        if (!integrity_verify_region(i, now))
            unchanged = 0;              // look at every region, log each one
    return unchanged;
}
