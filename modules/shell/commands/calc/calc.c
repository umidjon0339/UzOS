/*
 * calc.c - A small "recursive descent" parser for integer arithmetic.
 *
 * The grammar, from loosest to tightest binding:
 *
 *   expression = term   { ("+" | "-") term }
 *   term       = factor { ("*" | "/" | "%") factor }
 *   factor     = ["-"] ( number | "(" expression ")" )
 *
 * Each rule is one function, and each calls the next, so "*" binds
 * tighter than "+" automatically: 2 + 3 * 4 = 14.
 *
 * Every step is done in 64 bits and checked by safe_arith before it is
 * accepted, so nothing can overflow or divide by zero.
 *
 * Uses: the defence safe_arith.
 */

#include "calc.h"
#include "def_safe_arith.h"

#define MAX_DEPTH 32            // "((((...": stop before the kernel stack does

typedef struct {
    const char *p;              // next character to read
    const char *error;          // first problem found, or NULL
    int depth;                  // how many '(' we are inside
} parser_t;

static int64_t expression(parser_t *ps);

static void skip_spaces(parser_t *ps) {
    while (*ps->p == ' ')
        ps->p++;
}

static int64_t fail(parser_t *ps, const char *why) {
    if (!ps->error)
        ps->error = why;
    return 0;
}

// Digits -> value. Stops growing just past the int range (the caller
// checks the range), so even "99999999999999999999" cannot overflow.
static int64_t number(parser_t *ps) {
    if (*ps->p < '0' || *ps->p > '9')
        return fail(ps, "expected a number");

    int64_t value = 0;
    for (; *ps->p >= '0' && *ps->p <= '9'; ps->p++)
        if (value <= INT32_MAX)
            value = value * 10 + (*ps->p - '0');
    return value;
}

static int64_t factor(parser_t *ps) {
    // Any number of minus signs, counted in a loop (not by recursion).
    int negative = 0;
    for (skip_spaces(ps); *ps->p == '-'; skip_spaces(ps)) {
        negative = !negative;
        ps->p++;
    }

    int64_t value;
    if (*ps->p == '(') {
        if (++ps->depth > MAX_DEPTH)
            return fail(ps, "too many nested brackets");
        ps->p++;
        value = expression(ps);
        skip_spaces(ps);
        if (*ps->p != ')')
            return fail(ps, "missing ')'");
        ps->p++;
        ps->depth--;
    } else {
        value = number(ps);
    }
    if (ps->error)
        return 0;

    value = negative ? -value : value;
    // DEFENCE safe_arith: "99999999999" or "2147483648" does not fit an int.
    if (!def_calc_fits(value))
        return fail(ps, "number too big for 32 bits");
    return value;
}

static int64_t term(parser_t *ps) {
    int64_t value = factor(ps);
    for (;;) {
        skip_spaces(ps);
        char op = *ps->p;
        if (op != '*' && op != '/' && op != '%')
            return value;
        ps->p++;
        int64_t right = factor(ps);
        if (ps->error)
            return 0;

        if (op == '*') {
            value *= right;     // both fit 32 bits, so this fits 64
        } else {
            // DEFENCE safe_arith: no divide by zero, no INT_MIN / -1.
            if (!def_calc_divide_ok(value, right))
                return fail(ps, right == 0 ? "division by zero" : "result too big for 32 bits");
            value = (op == '/') ? value / right : value % right;
        }
        // DEFENCE safe_arith: the result must fit a 32-bit int.
        if (!def_calc_fits(value))
            return fail(ps, "result too big for 32 bits");
    }
}

static int64_t expression(parser_t *ps) {
    int64_t value = term(ps);
    for (;;) {
        skip_spaces(ps);
        char op = *ps->p;
        if (op != '+' && op != '-')
            return value;
        ps->p++;
        int64_t right = term(ps);
        if (ps->error)
            return 0;

        value = (op == '+') ? value + right : value - right;
        // DEFENCE safe_arith: the result must fit a 32-bit int.
        if (!def_calc_fits(value))
            return fail(ps, "result too big for 32 bits");
    }
}

int calc_eval(const char *text, int32_t *result, const char **error) {
    parser_t ps = { text, 0, 0 };
    int64_t value = expression(&ps);

    skip_spaces(&ps);
    if (!ps.error && *ps.p)
        ps.error = "unexpected character";
    if (!ps.error && !def_calc_fits(value))
        ps.error = "result too big for 32 bits";     // e.g. "-(-2147483648)"

    if (ps.error) {
        *error = ps.error;
        return 0;
    }
    *result = (int32_t)value;
    return 1;
}
