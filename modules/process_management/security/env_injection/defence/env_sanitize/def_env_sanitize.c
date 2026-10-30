/*
 * def_env_sanitize.c - DEFENCE env_sanitize: the new program's
 * environment is clean.
 *
 * Stops:  env_injection (security/env_injection)
 * Called: sys_execve() (syscalls/execve), for every envp entry, as it
 *         builds the new program's stack.
 * Rules:  1. NAME=value, NAME made of letters, digits and '_' and not
 *            starting with a digit
 *         2. only printable characters: no escape sequences
 *         3. names that change how a program is loaded or how a shell
 *            runs are dropped: LD_PRELOAD, LD_LIBRARY_PATH, LD_AUDIT,
 *            LD_DEBUG, IFS, BASH_ENV, ENV, PS4
 * Why:    the environment is input the new program did not choose.
 *         LD_PRELOAD loads attacker code into a privileged program
 *         before its main(); IFS and BASH_ENV change what a shell
 *         script does. Linux's dynamic linker ignores LD_* for setuid
 *         programs ("secure exec"), and sudo resets the environment.
 *         UzOS has no setuid programs and no dynamic linker; its policy
 *         is stricter and simpler: every exec is a secure exec.
 *
 * Uses: strncmp (lib/string), audit_log, registry.
 */

#include "modules/process_management/security/env_injection/defence/env_sanitize/def_env_sanitize.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "lib/string/string.h"                                    // strncmp, strlen

static const char *const dangerous[] = {
    "LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "LD_DEBUG",
    "IFS", "BASH_ENV", "ENV", "PS4",
};
#define DANGEROUS_COUNT (int)(sizeof(dangerous) / sizeof(dangerous[0]))

static int name_char(char c, int first) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' ||
           (!first && c >= '0' && c <= '9');
}

static const char *refusal(const char *e) {
    int n = 0;
    while (e[n] && e[n] != '=') {
        if (!name_char(e[n], n == 0))
            return "environment entry with a bad name";
        n++;
    }
    if (n == 0 || e[n] != '=')
        return "environment entry is not NAME=value";
    for (const char *c = e; *c; c++)
        if (*c < 0x20 || *c > 0x7E)
            return "environment entry with control characters";
    for (int i = 0; i < DANGEROUS_COUNT; i++)
        if ((int)strlen(dangerous[i]) == n && strncmp(e, dangerous[i], n) == 0)
            return "dangerous environment variable dropped (LD_PRELOAD, IFS, ...)";
    return 0;
}

int def_env_keep(const char *entry) {
    const char *why = refusal(entry);
    if (!why)
        return 1;
    audit_log("env_sanitize", why);
    return 0;
}

static int selftest(void) {
    if (refusal("HOME=/home/guest")) return 0;
    if (refusal("PATH_2=")) return 0;                   // empty value: fine
    if (!refusal("LD_PRELOAD=/tmp/evil.so")) return 0;
    if (!refusal("IFS=/")) return 0;
    if (!refusal("NOEQUALS")) return 0;
    if (!refusal("=value")) return 0;
    if (!refusal("1ST=x")) return 0;
    if (!refusal("TERM=\x1b[2J")) return 0;
    if (refusal("LD_PRELOADX=1")) return 0;             // a different name
    return 1;
}

static const defence_t defence = {
    "env_sanitize",
    "execve: only NAME=value, printable, never LD_PRELOAD / IFS / ...",
    "env_injection",
    selftest
};

void def_env_sanitize_register(void) { registry_add_defence(&defence); }
