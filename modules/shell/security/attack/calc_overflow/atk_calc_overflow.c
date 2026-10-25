/*
 * atk_calc_overflow.c - ATTACK calc_overflow: crash the kernel with maths.
 *
 * Breaks:     liveness - no typed input may halt the OS
 * Stopped by: safe_arith (security/defence/safe_arith)
 *
 * simulate(): list the inputs and what each would do.
 * launch():   evaluate each with the real calc_eval(); a normal
 *             expression must still work.
 *
 * Uses: calc_eval (commands/calc).
 */

#include "registry.h"
#include "audit.h"
#include "calc.h"
#include "vga.h"

static const struct { const char *input; const char *effect; } inputs[] = {
    { "-2147483648 / -1", "CPU exception 0: the kernel halts" },
    { "5 / 0",            "CPU exception 0: the kernel halts" },
    { "7 % 0",            "CPU exception 0: the kernel halts" },
    { "2147483647 + 1",   "wraps to -2147483648 (wrong answer)" },
    { "65536 * 65536",    "wraps to 0 (wrong answer)" },
    { "99999999999",      "a number no int can hold" },
};
#define INPUT_COUNT (sizeof(inputs) / sizeof(inputs[0]))

static int simulate(void) {
    for (unsigned i = 0; i < INPUT_COUNT; i++) {
        vga_print_str("  calc ");
        vga_print_str(inputs[i].input);
        vga_print_str("  -> ");
        vga_print_str(inputs[i].effect);
        vga_print_str("\n");
    }
    return INPUT_COUNT;
}

static int launch(void) {
    uint32_t denied = audit_total();
    int32_t result;
    const char *error;

    for (unsigned i = 0; i < INPUT_COUNT; i++) {
        int ok = calc_eval(inputs[i].input, &result, &error);
        vga_print_str("  ");
        vga_print_str(inputs[i].input);
        vga_print_str(ok ? "  -> COMPUTED\n" : "  -> refused\n");
    }

    int ok = calc_eval("(2 + 3) * 4", &result, &error);
    vga_print_str("  (2 + 3) * 4  -> ");
    if (ok)
        vga_print_int(result);
    else
        vga_print_str("FAILED");
    vga_print_str("\n");
    return audit_total() - denied;
}

static const attack_t attack = {
    "calc_overflow",
    "calc input that divides by zero or overflows halts the kernel",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, 0,        // only calculations: nothing to undo
    "safe_arith"
};

void atk_calc_overflow_register(void) {
    registry_add_attack(&attack);
}
