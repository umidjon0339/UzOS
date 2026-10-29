/*
 * def_signal_permission.c - DEFENCE signal_permission: who may signal whom.
 *
 * Stops:  signal_abuse (security/signal_abuse)
 * Called: the signal-sending paths (the `kill` command today; the kill /
 *         tkill / tgkill syscalls when they are added), before a signal
 *         reaches another process.
 * Rule:   a system process is never signalled; CAP_KILL may signal any
 *         other; otherwise you may signal only a process you own.
 * Why:    a signal can stop or disturb a process. Without an owner check
 *         any user could SIGINT or kill another user's work, or poke at
 *         the kernel's own processes. (kill_guard is the same idea for
 *         the `kill` command; this is the general rule for every signal.)
 */

#include "def_signal_permission.h"
#include "registry.h"
#include "audit.h"
#include "caps.h"               // caps_has, CAP_KILL

static int allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (target_is_system) return 0;                 // kernel processes: never
    if (caps_has(caller_uid, CAP_KILL)) return 1;   // root / privileged
    return caller_uid == target_uid;                // your own only
}

int def_signal_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (allowed(caller_uid, target_uid, target_is_system))
        return 1;
    audit_log("signal_permission", "signal refused: not yours, or a system process");
    return 0;
}

static int selftest(void) {
    if (!allowed(1, 1, 0)) return 0;        // your own process
    if (allowed(1, 2, 0)) return 0;         // someone else's
    if (allowed(1, 1, 1)) return 0;         // a system process, even your own uid
    if (!allowed(0, 2, 0)) return 0;        // root may signal any user process
    if (allowed(0, 0, 1)) return 0;         // not even root signals a system process
    return 1;
}

static const defence_t defence = {
    "signal_permission",
    "a signal reaches only your own processes (CAP_KILL for others)",
    "signal_abuse",
    selftest
};

void def_signal_permission_register(void) { registry_add_defence(&defence); }
