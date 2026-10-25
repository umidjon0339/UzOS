#ifndef INTEGRITY_H
#define INTEGRITY_H

/*
 * integrity.h - Has the kernel's code been changed since boot?
 *
 * Two parts of the kernel never change once it runs:
 *
 *   code        .text     the instructions
 *   constants   .rodata   strings, tables, the syscall table
 *
 * At boot we MEASURE them: take their SHA-256 (user_management/core/hash)
 * and keep it. Later we measure again and compare. One changed byte
 * gives a completely different hash, so a patched instruction (say, a
 * check turned into "always yes") cannot hide.
 *
 *     boot:   at_boot = SHA-256(code)                 integrity_init()
 *     later:  now     = SHA-256(code)
 *             now == at_boot ?                        integrity_verify()
 *                 yes: unchanged
 *                 no:  someone wrote into the kernel  (defence integrity_check)
 *
 * The kernel checks before `exec` starts a program, and `integrity`
 * checks when you ask. This DETECTS a change; it does not prevent it
 * (UzOS maps its code writable, see the README).
 */

#include <stdint.h>
#include "sha256.h"

#define INTEGRITY_REGIONS 2

typedef struct {
    const char    *name;                // "code"
    const char    *section;             // ".text"
    const char    *alarm;               // the audit log's text when it changed
    const uint8_t *start;
    const uint8_t *end;
    uint8_t        at_boot[SHA256_SIZE];
} integrity_region_t;

// Measure every region. Called once at boot, before anything could
// have touched them.
void integrity_init(void);

int integrity_region_count(void);
const integrity_region_t *integrity_region(int index);

// Measure region 'index' now (into 'now') and compare with its boot hash.
// 1 = unchanged. 0 = changed: the defence integrity_check logs it.
int integrity_verify_region(int index, uint8_t now[SHA256_SIZE]);

// Every region. 1 = the whole kernel is unchanged.
int integrity_verify(void);

#endif
