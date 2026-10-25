/*
 * def_input_bound.c - DEFENCE input_bound: typing never overflows the line.
 *
 * Stops:  line_overflow (security/attack/line_overflow)
 * Called: line_insert() (core/line), before each character is stored.
 * Rule:   used + 1 < capacity  (one slot always stays for the final NUL)
 * Why:    the line is a fixed array of 256 bytes. Storing the 257th
 *         character writes past the end, into whatever sits next to it
 *         in memory: the classic "buffer overflow", the bug behind many
 *         famous attacks. Keyboards can type forever; arrays cannot.
 */

#include "def_input_bound.h"
#include "registry.h"
#include "audit.h"

// The whole rule, as a pure function so the selftest can try it.
static int has_room(int used, int capacity) {
    return used >= 0 && used + 1 < capacity;
}

int def_input_room_ok(int used, int capacity) {
    if (has_room(used, capacity))
        return 1;
    audit_log("input_bound", "input line full: extra characters dropped");
    return 0;
}

static int selftest(void) {
    if (!has_room(0, 256)) return 0;        // empty line
    if (!has_room(254, 256)) return 0;      // the last character that fits
    if (has_room(255, 256)) return 0;       // slot 255 is for the NUL
    if (has_room(300, 256)) return 0;       // already past (never happens)
    if (has_room(-1, 256)) return 0;        // nonsense length
    return 1;
}

static const defence_t defence = {
    "input_bound",
    "never store more typed characters than the line buffer holds",
    "line_overflow",
    selftest
};

void def_input_bound_register(void) {
    registry_add_defence(&defence);
}
