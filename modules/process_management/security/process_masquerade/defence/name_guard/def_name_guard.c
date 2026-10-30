/*
 * def_name_guard.c - DEFENCE name_guard: a process may not pose as another.
 *
 * Stops:  process_masquerade (security/process_masquerade)
 * Called: sys_prctl(PR_SET_NAME) (syscalls/prctl), before a process
 *         changes the name `ps`, `top` and `pgrep` show.
 * Rules:  1. printable characters only (0x20..0x7E): no ANSI escapes,
 *            no newlines that would fake extra lines in `ps`
 *         2. not empty
 *         3. a normal process may not take a system process's name
 *            ("idle", "kernel_main")
 * Why:    people and tools find processes by name. Malware renames
 *         itself to look like part of the kernel, so nobody kills it;
 *         an escape sequence in a name can repaint the screen of the
 *         admin who runs `ps`.
 *
 * Uses: process_get_table (core/process), audit_log, registry
 *       (security_management), strcmp (lib/string).
 */

#include "modules/process_management/security/process_masquerade/defence/name_guard/def_name_guard.h"  // this file
#include "modules/security_management/core/registry/registry.h"           // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                 // audit_log
#include "lib/string/string.h"                                            // strcmp

static const char *refusal(const char *name, int caller_is_system,
                           const char *const *system_names, int count) {
    if (!name[0])
        return "empty process name";
    for (const char *c = name; *c; c++)
        if (*c < 0x20 || *c > 0x7E)
            return "process name with control characters";
    if (!caller_is_system)
        for (int i = 0; i < count; i++)
            if (strcmp(name, system_names[i]) == 0)
                return "process name copies a system process";
    return 0;
}

int def_name_ok(const process_t *p, const char *name) {
    const char *names[MAX_PROCESSES];
    int count = 0;
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].is_system && table[i].state != PROCESS_STATE_FREE)
            names[count++] = table[i].name;

    const char *why = refusal(name, p->is_system, names, count);
    if (!why)
        return 1;
    audit_log("name_guard", why);
    return 0;
}

static int selftest(void) {
    const char *sys[] = { "idle", "kernel_main" };
    if (refusal("worker", 0, sys, 2)) return 0;             // a normal name
    if (!refusal("kernel_main", 0, sys, 2)) return 0;       // posing as the kernel
    if (!refusal("idle", 0, sys, 2)) return 0;
    if (!refusal("a\x1b[2Jb", 0, sys, 2)) return 0;         // escape: clear screen
    if (!refusal("x\ny", 0, sys, 2)) return 0;              // a fake second line
    if (!refusal("", 0, sys, 2)) return 0;
    if (refusal("kernel_main", 1, sys, 2)) return 0;        // the kernel itself may
    return 1;
}

static const defence_t defence = {
    "name_guard",
    "process names: printable, and never a system process's name",
    "process_masquerade",
    selftest
};

void def_name_guard_register(void) { registry_add_defence(&defence); }
