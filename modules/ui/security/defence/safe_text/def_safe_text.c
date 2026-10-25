/*
 * def_safe_text.c - DEFENCE safe_text: a name can only ever be a name.
 *
 * Stops:  output_spoof (security/attack/output_spoof)
 * Called: ui_cell() (core/box), which tables use for every name they
 *         did not write themselves: process names in ps/top, and more.
 * Rule:   every character below 32 (and 127) is shown as '?';
 *         text longer than its column is cut, ending in '»'
 * Why:    a table trusts that each cell is one short piece of text. A
 *         process named "x\n  1  kernel_main  RUNNING" would print a
 *         NEW line that looks exactly like a real process: the table now
 *         lies. A very long name pushes the next columns out of place.
 *         Cutting is normal (not logged); a control character in a name
 *         is never normal, so that is logged.
 */

#include "def_safe_text.h"
#include "registry.h"
#include "audit.h"

#define CUT_MARK '\xAF'         // » in code page 437

static int is_control(char c) {
    return (unsigned char)c < 32 || (unsigned char)c == 127;
}

// The whole rule, as a pure function so the selftest can try it.
// Returns the number of control characters replaced.
static int clean(const char *text, char *out, int width) {
    int n = 0, replaced = 0;
    for (; text[n] && n < width; n++) {
        out[n] = is_control(text[n]) ? '?' : text[n];
        replaced += is_control(text[n]);
    }
    if (text[n] && n > 0)
        out[n - 1] = CUT_MARK;  // there was more: show that it was cut
    out[n] = '\0';
    return replaced;
}

int def_safe_text(const char *text, char *out, int width) {
    if (width < 0)
        width = 0;
    if (clean(text, out, width) == 0)
        return 1;
    audit_log("safe_text", "control character in a name shown as '?'");
    return 0;
}

static int same(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int selftest(void) {
    char out[16];
    if (clean("init", out, 8) != 0 || !same(out, "init")) return 0;
    if (clean("a\nb", out, 8) != 1 || !same(out, "a?b")) return 0;       // new line
    if (clean("\033[2J", out, 8) != 1 || !same(out, "?[2J")) return 0;   // escape code
    if (clean("abcdefghij", out, 5) != 0 || !same(out, "abcd\xAF")) return 0;  // cut
    if (clean("abcde", out, 5) != 0 || !same(out, "abcde")) return 0;   // fits exactly
    if (clean("", out, 5) != 0 || out[0]) return 0;
    return 1;
}

static const defence_t defence = {
    "safe_text",
    "show names in tables cleaned (no control characters) and cut to fit",
    "output_spoof",
    selftest
};

void def_safe_text_register(void) {
    registry_add_defence(&defence);
}
