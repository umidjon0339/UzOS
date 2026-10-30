/*
 * def_secure_umask.c - DEFENCE secure_umask: nothing is born writable by everyone.
 *
 * Stops:  umask_weak (security/umask_weak)
 * Called: ramdisk_create() (core/ramdisk), for every new file and directory.
 * Rule:   new mode = base & ~umask, and the "others may write" bit is
 *         always cleared. umask 0 cannot undo it; a later explicit
 *         `chmod 66` can, because then someone decided it on purpose.
 * Why:    a program inherits its umask from whoever started it. Start a
 *         root program with umask 0 and every file it creates (a log,
 *         a config, a script it runs later) is writable by every user:
 *         one of them edits the script, root runs it. Distributions set
 *         umask 022 (or 027) for the same reason; UzOS makes the
 *         world-writable part a rule, not a default.
 *
 * Uses: audit_log (a weak umask it overrode), registry (security_management).
 */

#include "modules/filesystem_management/security/umask_weak/defence/secure_umask/def_secure_umask.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

#define OTHERS_WRITE 002

static uint32_t creation_mode(uint32_t base, uint32_t umask) {
    return base & ~umask & ~OTHERS_WRITE & 066;
}

uint32_t def_creation_mode(uint32_t base, uint32_t umask) {
    if (base & ~umask & OTHERS_WRITE)           // the umask would have allowed it
        audit_log("secure_umask", "weak umask: others-write left out of a new file");
    return creation_mode(base, umask);
}

static int selftest(void) {
    if (creation_mode(066, 002) != 064) return 0;       // the normal case: rw/r-
    if (creation_mode(066, 000) != 064) return 0;       // umask 0: still no others-write
    if (creation_mode(066, 006) != 060) return 0;       // a private umask works
    if (creation_mode(066, 066) != 000) return 0;
    return 1;
}

static const defence_t defence = {
    "secure_umask",
    "a new file is never writable by others, whatever the umask",
    "umask_weak",
    selftest
};

void def_secure_umask_register(void) { registry_add_defence(&defence); }
