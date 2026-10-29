/*
 * def_window_quota.c - DEFENCE window_quota: only so many windows at once.
 *
 * Stops:  window_exhaustion (security/window_exhaustion)
 * Called: wm_create_window() (gui/wm/window_manager.c), before a slot
 *         is taken.
 * Rule:   used < MAX_WINDOWS.
 * Why:    the window table has MAX_WINDOWS fixed slots. Without a cap a
 *         program could ask for window after window until the table (and
 *         the heap behind each canvas) is exhausted and nothing else can
 *         open one.
 */

#include "def_window_quota.h"
#include "registry.h"
#include "audit.h"
#include "window_manager.h"        // MAX_WINDOWS

static int quota_ok(int used) {
    return used < MAX_WINDOWS;
}

int def_window_quota_ok(int used) {
    if (quota_ok(used))
        return 1;
    audit_log("window_quota", "window refused: every slot is in use");
    return 0;
}

static int selftest(void) {
    if (!quota_ok(0)) return 0;
    if (!quota_ok(MAX_WINDOWS - 1)) return 0;
    if (quota_ok(MAX_WINDOWS)) return 0;        // full
    if (quota_ok(MAX_WINDOWS + 5)) return 0;    // over
    return 1;
}

static const defence_t defence = {
    "window_quota",
    "refuse a new window once all MAX_WINDOWS slots are in use",
    "window_exhaustion",
    selftest
};

void def_window_quota_register(void) { registry_add_defence(&defence); }
