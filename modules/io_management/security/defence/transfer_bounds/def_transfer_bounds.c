/*
 * def_transfer_bounds.c - DEFENCE transfer_bounds: a sane buffer and size.
 *
 * Stops:  oversized_transfer (security/attack/oversized_transfer)
 * Called: sys_read and sys_write (syscalls/), before any byte is copied.
 * Rules:  count == 0 is always fine (nothing to copy). Otherwise:
 *         1. count <= IO_MAX_TRANSFER (16 KiB)
 *         2. buf is not in the NULL page
 *         3. buf + count does not wrap past 4 GiB
 * Why:    read() copies 'count' bytes to 'buf'. A NULL buffer, a huge
 *         count, or a range that wraps around to address 0 makes that
 *         copy write over memory the caller never owned.
 *         (When the call comes from ring 3, the int 0x80 handler has
 *         already checked that the buffer is the caller's own memory.)
 */

#include "def_transfer_bounds.h"
#include "security_lab.h"
#include "memory_layout.h"
#include "fd.h"

// The whole rule, as a pure function so the selftest can try it.
static const char *refusal(uint32_t buf, uint32_t count) {
    if (count == 0)                     return 0;
    if (count > IO_MAX_TRANSFER)        return "transfer larger than 16 KiB";
    if (buf < NULL_PAGE_END)            return "NULL buffer";
    if (count - 1 > 0xFFFFFFFFu - buf)  return "buffer wraps past 4 GiB";
    return 0;
}

int def_transfer_ok(const void *buf, uint32_t count) {
    const char *reason = refusal((uint32_t)buf, count);
    if (!reason)
        return 1;
    sec_log("transfer_bounds", reason);
    return 0;
}

static int selftest(void) {
    if (refusal(0x00200000, 64) != 0) return 0;                 // normal
    if (refusal(0, 0) != 0) return 0;                           // nothing to copy
    if (refusal(0, 16) == 0) return 0;                          // NULL
    if (refusal(0x00200000, IO_MAX_TRANSFER + 1) == 0) return 0;// too big
    if (refusal(0xFFFFFFF0, 32) == 0) return 0;                 // wraps
    if (refusal(0xFFFFFFF0, 16) != 0) return 0;                 // ends exactly at 4 GiB
    return 1;
}

static const defence_t defence = {
    "transfer_bounds",
    "refuse NULL, oversized or wrapping read/write buffers",
    "oversized_transfer",
    selftest
};

void def_transfer_bounds_register(void) {
    proc_security_register_defence(&defence);
}
