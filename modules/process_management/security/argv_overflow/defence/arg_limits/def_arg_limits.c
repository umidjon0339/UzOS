/*
 * def_arg_limits.c - DEFENCE arg_limits: execve's arguments have a size.
 *
 * Stops:  argv_overflow (security/argv_overflow)
 * Called: sys_execve() (syscalls/execve), before it copies argv and envp.
 * Rules:  1. at most 32 strings (argv + envp) and 2048 bytes together
 *         2. from ring 3: every array slot and every string is the
 *            caller's own mapped memory (user_ptr_bounds), and every
 *            string ends inside that memory
 * Why:    execve copies argv and envp into the kernel and then onto the
 *         new program's stack. Without a limit, a program passes a
 *         million arguments: the kernel buffer overflows, or the new
 *         stack is all arguments and the program dies at once (or
 *         worse, its stack runs into other memory - the "stack clash").
 *         A pointer into the kernel would make execve copy kernel
 *         memory into the new program. Linux: ARG_MAX / E2BIG.
 *
 * Uses: def_user_range_ok / def_user_string_ok (memory_management
 *       defence user_ptr_bounds), strlen, audit_log, registry.
 */

#include "modules/process_management/security/argv_overflow/defence/arg_limits/def_arg_limits.h"  // this file
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"         // def_user_range_ok, def_user_string_ok
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "lib/string/string.h"                                    // strlen

static int within(uint32_t count, uint32_t bytes) {
    return count <= ARG_MAX_STRINGS && bytes <= ARG_MAX_BYTES;
}

int def_args_ok(char *const list[], int from_user, uint32_t *count, uint32_t *bytes) {
    uint32_t n = *count, b = *bytes;
    for (int i = 0; list; i++) {
        if (from_user && !def_user_range_ok((uint32_t)&list[i], sizeof(char *)))
            return 0;                                       // logged by user_ptr_bounds
        const char *s = list[i];
        if (!s)
            break;
        if (from_user && !def_user_string_ok((uint32_t)s, ARG_MAX_BYTES))
            return 0;
        n++;
        b += strlen(s) + 1;
        if (!within(n, b)) {
            audit_log("arg_limits", "execve refused: too many / too long arguments (E2BIG)");
            return 0;
        }
    }
    *count = n;
    *bytes = b;
    return 1;
}

static int selftest(void) {
    if (!within(3, 40)) return 0;
    if (!within(ARG_MAX_STRINGS, ARG_MAX_BYTES)) return 0;      // exactly at the limit
    if (within(ARG_MAX_STRINGS + 1, 10)) return 0;              // one string too many
    if (within(2, ARG_MAX_BYTES + 1)) return 0;                 // one byte too many
    char *two[] = { "a", "b", 0 };
    uint32_t c = 0, by = 0;
    if (!def_args_ok(two, 0, &c, &by) || c != 2 || by != 4) return 0;   // counts right
    return 1;
}

static const defence_t defence = {
    "arg_limits",
    "execve: at most 32 strings / 2048 bytes; every pointer the caller's own",
    "argv_overflow",
    selftest
};

void def_arg_limits_register(void) { registry_add_defence(&defence); }
