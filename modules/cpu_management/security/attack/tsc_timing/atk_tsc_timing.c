/*
 * atk_tsc_timing.c - ATTACK tsc_timing: measure the kernel from ring 3.
 *
 * Breaks:     disclosure - how long the kernel takes must not leak secrets
 * Stopped by: tsc_restrict (security/defence/tsc_restrict)
 *
 * The kernel often does more work in one case than in another. Looking
 * up a file that exists walks further than one that does not. A program
 * that can time the kernel finely enough sees the difference, and so
 * learns things it was never told: a TIMING SIDE CHANNEL.
 *
 * simulate(): from ring 0 (where rdtsc is allowed), time vfs_lookup()
 *             for a file that exists and one that does not: the gap is
 *             what a ring 3 program would see.
 * launch():   run tsc_timing.s as user 1 in ring 3. It tries to time a
 *             getpid() syscall with rdtsc and leave the result in the
 *             usertask mailbox.
 *
 * Uses: tsc_now / tsc_cycles_to_ns (core/tsc), vfs_lookup / fs_root
 *       (filesystem_management), usertask_* (process_management/core/usertask).
 */

#include "security_lab.h"
#include "tsc.h"
#include "vfs.h"
#include "usertask.h"
#include "ui.h"

#define ATTACKER_UID 1
#define SAMPLES      64         // keep the fastest: interrupts only make it slower
#define WAIT_MS      300

// From tsc_timing.s
extern uint8_t tsc_timing_program[], tsc_timing_program_end[];

// Fastest of SAMPLES lookups of 'path', in cycles.
static uint64_t time_lookup(const char *path) {
    uint64_t best = ~0ull;
    for (int i = 0; i < SAMPLES; i++) {
        uint64_t start = tsc_now();
        vfs_lookup(fs_root, path);
        uint64_t took = tsc_now() - start;
        if (took < best)
            best = took;
    }
    return best;
}

static int simulate(void) {
    if (!tsc_available()) {
        ui_printf("  this CPU has no TSC: nothing to measure with\n");
        return 0;
    }
    uint64_t found   = time_lookup("/hello");
    uint64_t missing = time_lookup("/no_such_file");

    ui_printf("  vfs_lookup(\"/hello\")         %6u cycles  %6u ns  (exists)\n",
              (uint32_t)found, tsc_cycles_to_ns(found));
    ui_printf("  vfs_lookup(\"/no_such_file\")  %6u cycles  %6u ns  (missing)\n",
              (uint32_t)missing, tsc_cycles_to_ns(missing));
    ui_printf("  with rdtsc a program could see this gap; uptime() moves in 10 ms steps\n");
    return 1;
}

static int launch(void) {
    volatile uint32_t *mailbox = usertask_mailbox();

    int pid = usertask_start("tsctimer", tsc_timing_program, tsc_timing_program_end,
                             ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the ring 3 program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);

    if (mailbox[0] == 1) {
        ui_color(UI_BAD);
        ui_printf("  LEAK: ring 3 timed getpid() at %u cycles\n", mailbox[1]);
        ui_reset();
        return 1;
    }
    ui_color(UI_GOOD);
    ui_printf("  the program was stopped at its first rdtsc: no measurement\n");
    ui_reset();
    return 0;
}

static const attack_t attack = {
    "tsc_timing",
    "ring 3 reads the cycle counter to time the kernel (side channel)",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, 0,        // the program ends either way
    "tsc_restrict"
};

void atk_tsc_timing_register(void) {
    proc_security_register_attack(&attack);
}
