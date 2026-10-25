/*
 * def_safe_arith.c - DEFENCE safe_arith: arithmetic that cannot crash.
 *
 * Stops:  calc_overflow (security/attack/calc_overflow)
 * Called: calc_eval() (commands/calc), after every number and operation.
 * Rules:  1. every value and result fits a 32-bit int
 *         2. never divide by 0
 *         3. never divide INT_MIN by -1
 * Why:    on x86, a 32-bit division by 0, AND -2147483648 / -1 (the answer
 *         +2147483648 does not fit), raise CPU exception 0. In the kernel
 *         that halts the whole OS. And silent overflow gives wrong answers:
 *         2147483647 + 1 would print -2147483648.
 *         calc works in 64 bits and checks each step against these rules.
 */

#include "def_safe_arith.h"
#include "registry.h"
#include "audit.h"

// The rules, as pure functions so the selftest can try them.
static int fits(int64_t v) {
    return v >= INT32_MIN && v <= INT32_MAX;
}

static const char *divide_refusal(int64_t a, int64_t b) {
    if (b == 0)                     return "calc refused: division by zero";
    if (a == INT32_MIN && b == -1)  return "calc refused: INT_MIN / -1 overflows";
    return 0;
}

int def_calc_fits(int64_t value) {
    if (fits(value))
        return 1;
    audit_log("safe_arith", "calc refused: value does not fit 32 bits");
    return 0;
}

int def_calc_divide_ok(int64_t a, int64_t b) {
    const char *reason = divide_refusal(a, b);
    if (!reason)
        return 1;
    audit_log("safe_arith", reason);
    return 0;
}

static int selftest(void) {
    if (!fits(INT32_MAX) || !fits(INT32_MIN)) return 0;     // the edges fit
    if (fits((int64_t)INT32_MAX + 1)) return 0;             // one past
    if (fits((int64_t)INT32_MIN - 1)) return 0;
    if (divide_refusal(10, 0) == 0) return 0;               // divide by zero
    if (divide_refusal(INT32_MIN, -1) == 0) return 0;       // the overflow case
    if (divide_refusal(INT32_MIN, 1) != 0) return 0;        // fine
    if (divide_refusal(7, -1) != 0) return 0;               // fine
    return 1;
}

static const defence_t defence = {
    "safe_arith",
    "check every calc step: fits 32 bits, no /0, no INT_MIN / -1",
    "calc_overflow",
    selftest
};

void def_safe_arith_register(void) {
    registry_add_defence(&defence);
}
