#include "proc_security.h"

/* ===================================================================
 * DEFENCE: PID bounds
 * ===================================================================
 * Restores INV_IDENTITY - a PID must always name exactly one process.
 *
 * PIDs are handed out by a counter that only ever goes up. That is
 * already the right idea, but nothing stops the counter from reaching
 * the end of its 32-bit range and wrapping back to a small number that
 * is still in use. Two live processes would then share a PID, and
 * process_get_by_pid() would silently return the wrong one: "kill 7"
 * could kill a process you have never heard of.
 *
 * The guard is deliberately conservative. Running out of PIDs is a
 * clean, visible failure; a reused PID is a silent, dangerous one.
 * =================================================================== */

#define PID_LIMIT 0xFFFFFF00u   /* stop well before the counter wraps */

static int pid_in_range(uint32_t pid) {
    if (pid == 0) return 0;         /* 0 means "no process" */
    if (pid >= PID_LIMIT) return 0; /* wrapper would collide */
    return 1;
}

int def_pid_bounds_check(uint32_t pid) {
    if (pid_in_range(pid)) return 1;

    sec_log("pid_bounds", "PID out of range or space exhausted");
    return 0;
}

static int def_pid_bounds_selftest(void) {
    /* normal PIDs are fine */
    if (pid_in_range(1) != 1) return 0;
    if (pid_in_range(1000) != 1) return 0;

    /* PID 0 is reserved */
    if (pid_in_range(0) != 0) return 0;

    /* the counter must stop before it can wrap */
    if (pid_in_range(PID_LIMIT) != 0) return 0;
    if (pid_in_range(PID_LIMIT - 1) != 1) return 0;
    if (pid_in_range(0xFFFFFFFF) != 0) return 0;

    return 1;
}

static const defence_t def = {
    "pid_bounds",
    "stop the PID counter before it can wrap and collide",
    "pid_wrap",
    def_pid_bounds_selftest
};

void def_pid_bounds_register(void) {
    proc_security_register_defence(&def);
}
