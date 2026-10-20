/*
 * def_file_access.c - DEFENCE file_access: a file is only used as its mode allows.
 *
 * Stops:  file_permission_bypass (security/attack/file_permission_bypass)
 * Called: perm_check / perm_may_remove / perm_may_chmod (core/perm),
 *         which sys_open, sys_mkdir, sys_unlink, sys_rename, sys_chdir,
 *         `ls`, `chmod` and the ELF loader call before touching a file.
 * Rules:  1. root (uid 0) may do anything
 *         2. the owner gets the FIRST digit of the mode
 *         3. everybody else gets the SECOND digit
 *         4. the wanted bits (4 = read, 2 = write) must all be there
 *         Removing a file or changing its mode: owner or root only.
 * Why:    without this, every user can read every other user's files,
 *         overwrite them, or delete them: users would have names but
 *         nothing of their own.
 */

#include "def_file_access.h"
#include "security_lab.h"
#include "perm.h"
#include "users.h"

// The whole rule, as a pure function so the selftest can try it.
static int access_allows(uint32_t uid, uint32_t owner, uint32_t mode, int want) {
    if (uid == UID_ROOT) return 1;                          // rule 1
    uint32_t digit = uid == owner ? mode >> 3 : mode & 7;   // rules 2 and 3
    return (digit & (uint32_t)want) == (uint32_t)want;      // rule 4
}

int def_file_access_ok(uint32_t uid, uint32_t owner, uint32_t mode, int want) {
    if (access_allows(uid, owner, mode, want))
        return 1;
    sec_log("file_access", "permission denied by the file's mode");
    return 0;
}

int def_file_access_owner_ok(uint32_t uid, uint32_t owner) {
    if (uid == UID_ROOT || uid == owner)
        return 1;
    sec_log("file_access", "permission denied: not the owner");
    return 0;
}

static int selftest(void) {
    const uint32_t owner = 1000, other = 1001;

    if (!access_allows(owner, owner, 060, PERM_READ | PERM_WRITE)) return 0;
    if (access_allows(other, owner, 060, PERM_READ)) return 0;     // private file
    if (!access_allows(other, owner, 064, PERM_READ)) return 0;    // others may read
    if (access_allows(other, owner, 064, PERM_WRITE)) return 0;    // ...but not write
    if (access_allows(owner, owner, 040, PERM_WRITE)) return 0;    // owner made it read-only
    if (!access_allows(UID_ROOT, owner, 000, PERM_WRITE)) return 0; // root passes
    return 1;
}

static const defence_t defence = {
    "file_access",
    "files are used only as their owner and mode allow; root passes",
    "file_permission_bypass",
    selftest
};

void def_file_access_register(void) {
    proc_security_register_defence(&defence);
}
