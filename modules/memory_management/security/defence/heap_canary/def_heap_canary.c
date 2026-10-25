/*
 * def_heap_canary.c - DEFENCE heap_canary: spot an overwritten block header.
 *
 * Stops:  heap_overflow (security/attack/heap_overflow)
 * Called: kmalloc() and kfree() (core/kheap), for every header before
 *         they follow its 'next' pointer.
 * Rules:  1. the header holds KHEAP_MAGIC (the "canary")
 *         2. 'next' is NULL, or points further on inside the heap
 * Why:    writing past the end of a block lands in the next block's
 *         header. Its 'next' pointer would then send the heap code into
 *         random memory. The magic number works like a canary in a coal
 *         mine: if it is gone, something overwrote the header, so we stop
 *         walking instead of following garbage. kmalloc() then fails
 *         cleanly instead of crashing.
 */

#include "def_heap_canary.h"
#include "registry.h"
#include "audit.h"

// The whole rule, as a pure function so the selftest can try it.
static int header_intact(const kheap_header_t *h) {
    uint32_t self = (uint32_t)h;
    uint32_t next = (uint32_t)h->next;

    if (h->magic != KHEAP_MAGIC) return 0;                      // rule 1
    if (next == 0) return 1;                                    // last block
    return next > self && next < KHEAP_START + KHEAP_SIZE;      // rule 2
}

int def_heap_block_ok(const kheap_header_t *h) {
    if (header_intact(h))
        return 1;
    audit_log("heap_canary", "heap block header overwritten: walk stopped");
    return 0;
}

static int selftest(void) {
    // A fake header on our stack; only its fields matter.
    kheap_header_t h = { KHEAP_MAGIC, 64, 0, 0 };
    if (!header_intact(&h)) return 0;               // last block: ok

    h.magic = 0x41414141;                           // 'AAAA' from an overflow
    if (header_intact(&h)) return 0;

    h.magic = KHEAP_MAGIC;
    h.next = (kheap_header_t *)0x00100000;          // points into the kernel
    if (header_intact(&h)) return 0;
    return 1;
}

static const defence_t defence = {
    "heap_canary",
    "check each heap header's magic before following it",
    "heap_overflow",
    selftest
};

void def_heap_canary_register(void) {
    registry_add_defence(&defence);
}
