/*
 * def_file_range.c - DEFENCE file_range: writes stay inside a file's limit.
 *
 * Stops:  file_offset_wrap (security/attack/file_offset_wrap)
 * Called: ramdisk_write() (core/ramdisk), before memory is grown or copied.
 * Rule:   offset <= limit  and  size <= limit - offset
 * Why:    the obvious check "offset + size <= limit" wraps in 32 bits:
 *         0xFFFFFFF0 + 0x20 = 0x10, which passes, and the copy then
 *         writes 4 GiB past the file's buffer.
 */

#include "def_file_range.h"
#include "security_lab.h"

// The whole rule, as a pure function so the selftest can try it.
static int in_range(uint32_t offset, uint32_t size, uint32_t limit) {
    return offset <= limit && size <= limit - offset;   // no addition: no wrap
}

int def_file_range_ok(uint32_t offset, uint32_t size, uint32_t limit) {
    if (in_range(offset, size, limit))
        return 1;
    sec_log("file_range", "file write past the size limit refused");
    return 0;
}

static int selftest(void) {
    if (!in_range(0, 10, 10)) return 0;             // the whole file
    if (!in_range(9, 1, 10)) return 0;              // the last byte
    if (in_range(10, 1, 10)) return 0;              // one past the end
    if (in_range(0xFFFFFFF0, 0x20, 0x10000)) return 0;  // would wrap to 0x10
    if (in_range(11, 0, 10)) return 0;              // offset past the limit
    return 1;
}

static const defence_t defence = {
    "file_range",
    "check offset + size against the file limit without wrapping",
    "file_offset_wrap",
    selftest
};

void def_file_range_register(void) {
    proc_security_register_defence(&defence);
}
