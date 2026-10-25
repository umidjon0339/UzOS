/*
 * def_pmm_free_guard.c - DEFENCE pmm_free_guard: only real, used frames
 * can be freed.
 *
 * Stops:  pmm_bad_free (security/attack/pmm_bad_free)
 * Called: pmm_free_block() (core/pmm), before the bitmap is touched.
 * Rules:  1. the address is the start of a frame (multiple of 4096)
 *         2. the frame is real RAM (inside the bitmap's range)
 *         3. the frame is not reserved (kernel image, firmware, boot data)
 *         4. the frame is in use (freeing a free frame = double free)
 * Why:    a freed frame is handed to the next pmm_alloc_block() caller.
 *         Freeing a kernel frame means the kernel's own code gets
 *         overwritten; a double free hands one frame to two owners.
 */

#include "def_pmm_free_guard.h"
#include "registry.h"
#include "audit.h"
#include "pmm.h"

// Why a free was refused, so the log can say it.
static const char *refusal_reason(uint32_t addr, uint32_t total_frames,
                                  int reserved, int used) {
    if (addr % PAGE_SIZE)                return "frame free refused: not frame-aligned";
    if (addr / PAGE_SIZE >= total_frames) return "frame free refused: not real RAM";
    if (reserved)                        return "frame free refused: reserved (kernel/firmware)";
    if (!used)                           return "frame free refused: already free (double free)";
    return 0;                            // allowed
}

int def_pmm_free_ok(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    const char *reason = refusal_reason(addr, pmm_get_max_blocks(),
                                        pmm_is_reserved(frame), pmm_is_used(frame));
    if (!reason)
        return 1;
    audit_log("pmm_free_guard", reason);
    return 0;
}

static int selftest(void) {
    const uint32_t total = 1000;     // a pretend machine with 1000 frames

    if (refusal_reason(0x200000, total, 0, 1) != 0) return 0;       // normal free
    if (refusal_reason(0x200001, total, 0, 1) == 0) return 0;       // misaligned
    if (refusal_reason(total * PAGE_SIZE, total, 0, 1) == 0) return 0; // past RAM
    if (refusal_reason(0x100000, total, 1, 1) == 0) return 0;       // kernel frame
    if (refusal_reason(0x200000, total, 0, 0) == 0) return 0;       // double free
    return 1;
}

static const defence_t defence = {
    "pmm_free_guard",
    "free only aligned, real, non-reserved frames that are in use",
    "pmm_bad_free",
    selftest
};

void def_pmm_free_guard_register(void) {
    registry_add_defence(&defence);
}
