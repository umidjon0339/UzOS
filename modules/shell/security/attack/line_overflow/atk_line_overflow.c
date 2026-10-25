/*
 * atk_line_overflow.c - ATTACK line_overflow: type more than the line holds.
 *
 * Breaks:     isolation - typed text stays inside its buffer
 * Stopped by: input_bound (security/defence/input_bound)
 *
 * simulate(): explain the overflow.
 * launch():   make a test line with "guard" words right after it, feed it
 *             1000 characters through the real line_insert(), then check
 *             the length, the final NUL, and the guard words.
 *
 * Uses: line_t / line_clear / line_insert (core/line).
 */

#include "registry.h"
#include "audit.h"
#include "line.h"
#include "vga.h"

#define KEYS   1000
#define GUARDS 16
#define MARK   0xDEADBEEF

static int simulate(void) {
    vga_print_str("  the line is a 256-byte array; a keyboard never runs out of keys\n");
    vga_print_str("  key 257 would be written past the end, over the next variable\n");
    return 1;
}

static int launch(void) {
    // The line, with guard words placed right behind it in memory.
    static struct {
        line_t line;
        uint32_t guard[GUARDS];
    } test;

    uint32_t denied = audit_total();
    line_clear(&test.line);
    for (int i = 0; i < GUARDS; i++)
        test.guard[i] = MARK;

    int stored = 0;
    for (int i = 0; i < KEYS; i++)
        stored += line_insert(&test.line, 'A');

    int damaged = 0;
    for (int i = 0; i < GUARDS; i++)
        damaged += test.guard[i] != MARK;

    vga_print_str("  typed 1000 keys, stored ");
    vga_print_int(stored);
    vga_print_str(", line length ");
    vga_print_int(test.line.length);
    vga_print_str(test.line.text[test.line.length] == '\0' ? ", NUL at the end\n" : ", NO NUL\n");
    vga_print_str("  guard words damaged: ");
    vga_print_int(damaged);
    vga_print_str(damaged ? "  (OVERFLOW)\n" : "\n");
    return audit_total() - denied;
}

static const attack_t attack = {
    "line_overflow",
    "typing more characters than the line buffer holds",
    INV_ISOLATION, SEV_HIGH,
    simulate, launch, 0,        // a separate test line: the real one is untouched
    "input_bound"
};

void atk_line_overflow_register(void) {
    registry_add_attack(&attack);
}
