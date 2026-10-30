/*
 * atk_env_injection.c - ATTACK env_injection: smuggle behaviour in
 * through the environment.
 *
 * Breaks:     trust - a program decides what code it runs
 * Stopped by: env_sanitize (defence/env_sanitize)
 *
 * The environment is input a program did not choose. LD_PRELOAD makes
 * the dynamic linker load attacker code into the program before main();
 * IFS and BASH_ENV change what a shell script does; a "value" with an
 * escape sequence repaints a terminal; a malformed entry confuses
 * getenv(). Given to a privileged program, that is a takeover.
 *
 * simulate(): list the entries.
 * launch():   hand each to env_sanitize, as execve does for every envp
 *             entry. findings = harmful entries that would reach the
 *             program (the plain HOME= must pass).
 *             (`proccalls exec` runs a real program with them.)
 * cleanup():  nothing.
 *
 * Uses: def_env_keep (its defence), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/env_injection/defence/env_sanitize/def_env_sanitize.h"  // def_env_keep
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

static const struct {
    const char *entry;
    const char *shown;
    int harmful;
} entries[] = {
    { "LD_PRELOAD=/home/guest/evil.so", "LD_PRELOAD=/home/guest/evil.so", 1 },
    { "IFS=/",                          "IFS=/",                          1 },
    { "BASH_ENV=/home/guest/x.sh",      "BASH_ENV=/home/guest/x.sh",      1 },
    { "TERM=\x1b[2Jxterm",              "TERM=ESC[2Jxterm",               1 },
    { "NOEQUALS",                       "NOEQUALS",                       1 },
    { "HOME=/home/guest",               "HOME=/home/guest",               0 },
};
#define ENTRY_COUNT (int)(sizeof(entries) / sizeof(entries[0]))

static int simulate(void) {
    for (int i = 0; i < ENTRY_COUNT; i++)
        ui_printf("  envp: %s\n", entries[i].shown);
    return ENTRY_COUNT - 1;
}

static int launch(void) {
    int passed_bad = 0;
    for (int i = 0; i < ENTRY_COUNT; i++) {
        int kept = def_env_keep(entries[i].entry);
        int bad = entries[i].harmful ? kept : !kept;
        passed_bad += entries[i].harmful && kept;
        ui_printf("  %-34s ", entries[i].shown);
        ui_color(bad ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", kept ? "reaches the program" : "dropped");
        ui_reset();
    }
    return passed_bad;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "env_injection",
    "execve with LD_PRELOAD, IFS, escape codes in the environment",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "env_sanitize"
};

void atk_env_injection_register(void) { registry_add_attack(&attack); }
