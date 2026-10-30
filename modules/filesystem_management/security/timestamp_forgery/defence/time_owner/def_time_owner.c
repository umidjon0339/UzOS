/*
 * def_time_owner.c - DEFENCE time_owner: only the owner chooses a file's time.
 *
 * Stops:  timestamp_forgery (security/timestamp_forgery)
 * Called: sys_utime, sys_utimes, sys_utimensat (syscalls/), before a
 *         node's atime / mtime change.
 * Rules:  1. setting CHOSEN times: the owner, or CAP_FILES (root)
 *         2. setting them to NOW: also anyone who may write the file
 *            (writing would set mtime to now anyway)
 *         3. ctime is never chosen: it always becomes now
 * Why:    times are evidence. "Who changed /etc/passwd, and when?" is
 *         answered from mtime and ctime. An intruder who could set any
 *         file's times back to last year would hide what they edited;
 *         backup and build tools that copy "only what changed" would
 *         skip it too. Linux has exactly these rules (and ctime cannot
 *         be set at all).
 *
 * Uses: process_caller_uid (process_management), caps_has / CAP_FILES
 *       (security_management), perm_check (user_management),
 *       audit_log, registry.
 */

#include "modules/filesystem_management/security/timestamp_forgery/defence/time_owner/def_time_owner.h"  // this file
#include "modules/process_management/core/process/process.h"        // process_caller_uid
#include "modules/security_management/core/caps/caps.h"             // caps_has, CAP_FILES
#include "modules/user_management/core/perm/perm.h"                 // perm_check, PERM_WRITE
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

static int allowed(int is_owner, int has_cap, int can_write, int explicit_times) {
    if (is_owner || has_cap)
        return 1;
    return !explicit_times && can_write;
}

int def_utime_ok(const vfs_node_t *node, int explicit_times) {
    uint32_t uid = process_caller_uid();
    int is_owner = node->owner == uid;
    int has_cap  = caps_has(uid, CAP_FILES);
    int can_write = !is_owner && !has_cap && !explicit_times && perm_check(node, PERM_WRITE);
    if (allowed(is_owner, has_cap, can_write, explicit_times))
        return 1;
    audit_log("time_owner", "file times refused: only the owner may choose them");
    return 0;
}

static int selftest(void) {
    if (!allowed(1, 0, 0, 1)) return 0;     // the owner sets any time
    if (allowed(0, 0, 1, 1)) return 0;      // a writer may not choose a time
    if (!allowed(0, 0, 1, 0)) return 0;     // ...but may set "now"
    if (allowed(0, 0, 0, 0)) return 0;      // a stranger: nothing
    if (!allowed(0, 1, 0, 1)) return 0;     // CAP_FILES
    return 1;
}

static const defence_t defence = {
    "time_owner",
    "chosen file times: owner only; \"now\": owner or a writer",
    "timestamp_forgery",
    selftest
};

void def_time_owner_register(void) { registry_add_defence(&defence); }
