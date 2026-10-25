/*
 * def_integrity_check.c - DEFENCE integrity_check: notice a patched kernel.
 *
 * Stops:  kernel_patch (security/attack/kernel_patch)
 * Called: integrity_verify() (core/integrity), which elf_load_and_run()
 *         calls before it starts a program, and the `integrity` command.
 * Rules:  1. a region was measured at boot (its hash is not all zero)
 *         2. its hash now is the same as at boot, byte for byte
 *         A changed kernel starts no more programs until it is rebooted.
 * Why:    code in ring 0 can write anywhere, also into the kernel's own
 *         instructions: one byte turns `if (!allowed) return 0;` into
 *         "always allowed". Every other defence trusts that the code
 *         it is made of is still the code that was built.
 */

#include "def_integrity_check.h"
#include "registry.h"
#include "audit.h"
#include "string.h"

// The whole rule, as a pure function so the selftest can try it.
static int hashes_match(const uint8_t at_boot[SHA256_SIZE], const uint8_t now[SHA256_SIZE]) {
    uint8_t measured = 0, differ = 0;
    for (int i = 0; i < SHA256_SIZE; i++) {
        measured |= at_boot[i];                 // rule 1
        differ   |= at_boot[i] ^ now[i];        // rule 2: look at every byte
    }
    return measured && !differ;
}

int def_integrity_unchanged(const char *alarm, const uint8_t at_boot[SHA256_SIZE],
                            const uint8_t now[SHA256_SIZE]) {
    if (hashes_match(at_boot, now))
        return 1;
    audit_log("integrity_check", alarm);
    return 0;
}

static int selftest(void) {
    uint8_t code[16] = "mov eax, 1; ret";
    uint8_t boot[SHA256_SIZE], now[SHA256_SIZE], never[SHA256_SIZE];

    sha256(code, sizeof(code), boot);
    sha256(code, sizeof(code), now);
    if (!hashes_match(boot, now)) return 0;     // same bytes: unchanged

    code[4] ^= 1;                               // one bit of one byte
    sha256(code, sizeof(code), now);
    if (hashes_match(boot, now)) return 0;      // must be noticed

    memset(never, 0, sizeof(never));
    if (hashes_match(never, never)) return 0;   // never measured: no proof
    return 1;
}

static const defence_t defence = {
    "integrity_check",
    "hash kernel code at boot; refuse to run programs once it changed",
    "kernel_patch",
    selftest
};

void def_integrity_check_register(void) {
    registry_add_defence(&defence);
}
