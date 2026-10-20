/*
 * def_heap_guard.c - DEFENCE heap_guard: kfree() only real, used blocks.
 *
 * Stops:  heap_double_free (security/attack/heap_double_free)
 * Called: kfree() (core/kheap), before the block is touched.
 * Rules:  1. p lies inside the heap, where a block's data can start
 *         2. p is 8-byte aligned, like every block kmalloc() returns
 *         3. the header just before p has the heap magic
 *         4. the block is in use (not already freed)
 * Why:    kfree() writes into the header just before p. With a pointer
 *         from outside the heap, it would write into random memory; with
 *         a double free, the block's list could end up linked twice.
 *         Rule 1 and 2 are checked first, so a bad pointer is never
 *         even read through.
 */

#include "def_heap_guard.h"
#include "security_lab.h"
#include "kheap.h"

#define HEADER_SIZE sizeof(kheap_header_t)

// Rules 1 and 2: only uses the address, never reads memory.
static int plausible_block(uint32_t p) {
    if (p < KHEAP_START + HEADER_SIZE) return 0;
    if (p >= KHEAP_START + KHEAP_SIZE) return 0;
    return p % 8 == 0;
}

// Rules 3 and 4: what the header says.
static int header_allows(const kheap_header_t *h) {
    return h->magic == KHEAP_MAGIC && !h->is_free;
}

int def_kfree_ok(const void *p) {
    uint32_t addr = (uint32_t)p;
    const char *reason = 0;

    if (!plausible_block(addr))
        reason = "kfree refused: pointer is not inside the heap";
    else if (!header_allows((const kheap_header_t *)(addr - HEADER_SIZE)))
        reason = "kfree refused: not a block start, or already freed";

    if (!reason)
        return 1;
    sec_log("heap_guard", reason);
    return 0;
}

static int selftest(void) {
    kheap_header_t h = { KHEAP_MAGIC, 64, 0, 0 };

    if (!plausible_block(KHEAP_START + HEADER_SIZE)) return 0;  // first block
    if (plausible_block(0x00100000)) return 0;                  // kernel code
    if (plausible_block(KHEAP_START + KHEAP_SIZE)) return 0;    // just past the heap
    if (plausible_block(KHEAP_START + HEADER_SIZE + 4)) return 0; // misaligned

    if (!header_allows(&h)) return 0;       // a used block: ok
    h.is_free = 1;
    if (header_allows(&h)) return 0;        // already free: double free
    h.is_free = 0;
    h.magic = 0x41414141;
    if (header_allows(&h)) return 0;        // no magic: not a block start
    return 1;
}

static const defence_t defence = {
    "heap_guard",
    "kfree only accepts real, in-use heap blocks",
    "heap_double_free",
    selftest
};

void def_heap_guard_register(void) {
    proc_security_register_defence(&defence);
}
