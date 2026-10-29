/*
 * def_pid_bounds.c - DEFENCE pid_bounds: a PID never names two processes.
 *
 * Stops:  pid_wrap (security/pid_wrap)
 * Called: process_create() for the next PID, process_kill() for the target.
 * Rule:   0 < pid < PID_LIMIT
 * Why:    PIDs come from a counter that only goes up. If it wrapped past
 *         0xFFFFFFFF back to 1, a new process could get the PID of a live
 *         one, and `kill 7` might hit the wrong process. Running out of
 *         PIDs is a visible error; a reused PID is a silent one.
 */

#include "def_pid_bounds.h"
#include "registry.h"
#include "audit.h"

// The whole rule, as a pure function so the selftest can try it.
static int pid_in_range(uint32_t pid) {
    return pid != 0 && pid < PID_LIMIT;     // 0 means "no process"
}

int def_pid_bounds_check(uint32_t pid) {
    if (pid_in_range(pid))
        return 1;

    audit_log("pid_bounds", "PID 0, out of range, or PID space used up");
    return 0;
}

static int selftest(void) {
    if (pid_in_range(1) != 1) return 0;
    if (pid_in_range(1000) != 1) return 0;
    if (pid_in_range(0) != 0) return 0;                 // reserved
    if (pid_in_range(PID_LIMIT - 1) != 1) return 0;     // last allowed
    if (pid_in_range(PID_LIMIT) != 0) return 0;         // first refused
    if (pid_in_range(0xFFFFFFFF) != 0) return 0;        // just before the wrap
    return 1;
}

static const defence_t defence = {
    "pid_bounds",
    "stop handing out PIDs before the counter can wrap",
    "pid_wrap",
    selftest
};

void def_pid_bounds_register(void) {
    registry_add_defence(&defence);
}
