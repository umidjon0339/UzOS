/*
 * def_tsc_restrict.c - DEFENCE tsc_restrict: the cycle counter is kernel-only.
 *
 * Stops:  tsc_timing (security/attack/tsc_timing)
 * Called: tsc_init() (core/tsc) switches it on at boot; fault_handler()
 *         (core/faults) reports each refused rdtsc.
 * Rule:   CR4.TSD = 1  ->  rdtsc in ring 3 raises #13 General Protection,
 *         and fault_isolation kills the program. Ring 0 is not affected.
 * Why:    a clock that counts single cycles is the first tool of a
 *         TIMING ATTACK. If the kernel takes a little longer for "file
 *         exists" than for "no such file", or for one key bit than for
 *         another, a program with rdtsc can see the difference and learn
 *         what it was never told. Programs still have uptime(), which
 *         moves in 10 ms steps: far too coarse for that.
 *         (Linux keeps rdtsc open for speed; seccomp can close it. Here
 *         we choose the safe side to show the switch.)
 */

#include "def_tsc_restrict.h"
#include "security_lab.h"
#include "cpu.h"
#include "cpu_info.h"
#include "string.h"

#define VECTOR_GP 13

// The whole rule, as a pure function so the selftest can try it.
static int rdtsc_allowed(uint32_t cr4, int ring) {
    return ring == 0 || !(cr4 & CR4_TSD);
}

void def_tsc_restrict_enable(void) {
    if (cpu_has(CPU_FEAT_TSC))          // no TSC: nothing to protect (and maybe no CR4)
        cpu_write_cr4(cpu_read_cr4() | CR4_TSD);
}

void def_tsc_restrict_note(uint8_t vector, const char *instruction) {
    if (vector == VECTOR_GP && instruction && strcmp(instruction, "rdtsc") == 0)
        sec_log("tsc_restrict", "rdtsc from ring 3 refused: the cycle counter is kernel-only");
}

static int selftest(void) {
    if (!rdtsc_allowed(0, 0)) return 0;             // kernel, TSD off
    if (!rdtsc_allowed(CR4_TSD, 0)) return 0;       // kernel, TSD on: still fine
    if (!rdtsc_allowed(0, 3)) return 0;             // user, TSD off: open (the old way)
    if (rdtsc_allowed(CR4_TSD, 3)) return 0;        // user, TSD on: refused

    // And the switch must really be on in this CPU right now.
    if (cpu_has(CPU_FEAT_TSC) && rdtsc_allowed(cpu_read_cr4(), 3))
        return 0;
    return 1;
}

static const defence_t defence = {
    "tsc_restrict",
    "set CR4.TSD: rdtsc in ring 3 faults, so programs get only a coarse clock",
    "tsc_timing",
    selftest
};

void def_tsc_restrict_register(void) {
    proc_security_register_defence(&defence);
}
