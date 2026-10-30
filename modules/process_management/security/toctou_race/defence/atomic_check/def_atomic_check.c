/*
 * def_atomic_check.c - DEFENCE atomic_check: check and use in one step.
 *
 * Stops:  toctou_race (security/toctou_race)
 * Called: the rule every check-then-use site should follow (e.g. open
 *         with O_CREAT|O_EXCL instead of "does it exist?" then create).
 * Rule:   do the check and the use as one operation the kernel cannot be
 *         interrupted in the middle of; never "look, then act" on shared
 *         state that another task could change between the two.
 * Why:    TOCTOU = Time Of Check to Time Of Use. If you test a file's
 *         owner and THEN open it, an attacker can swap the file in the
 *         gap and you open theirs. One atomic step removes the gap.
 */

#include "modules/process_management/security/toctou_race/defence/atomic_check/def_atomic_check.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                     // defence_t, registry_add_defence

static int atomic(int single_step) {
    return single_step != 0;
}

int def_check_is_atomic(int single_step) {
    return atomic(single_step);
}

static int selftest(void) {
    if (!atomic(1)) return 0;       // one step: safe
    if (atomic(0)) return 0;        // check-then-use: a race window
    return 1;
}

static const defence_t defence = {
    "atomic_check",
    "check and use in one uninterruptible step; no check-then-use gap",
    "toctou_race",
    selftest
};

void def_atomic_check_register(void) { registry_add_defence(&defence); }
