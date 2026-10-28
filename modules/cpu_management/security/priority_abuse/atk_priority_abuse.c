/*
 * atk_priority_abuse.c - ATTACK priority_abuse: ask for a priority you may not have.
 *
 * Breaks:     trust - a number from a user program is checked before use
 * Stopped by: priority_bounds (security/priority_abuse/defence/priority_bounds)
 *
 * setpriority(which, who, nice) and sched_setscheduler(pid, policy,
 * param) take every number from the caller. Without checks a user
 * program could:
 *   - pick a nice value no process should have (1000, -500)
 *   - lower its own nice to -20 and push everyone else back
 *   - renice the shell, or the idle task, slowing the whole OS
 *   - make itself SCHED_FIFO 99: it runs before every normal process
 *
 * simulate(): list the six calls and what each one would do.
 * launch():   run priority_out_of_range.s as user 1 in ring 3; it makes
 *             the six calls for real and leaves the answers in the
 *             usertask mailbox.
 *
 * Uses: usertask_* (process_management/core/usertask), and the syscalls
 *       setpriority and sched_setscheduler (cpu_management/syscalls)
 *       through int 0x80.
 */

#include "modules/security_management/core/registry/registry.h"    // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"     // usertask_start, usertask_mailbox
#include "modules/ui/core/style/ui.h"                              // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      300
#define DONE         0x600D         // what the program writes to mailbox[0]

// From priority_out_of_range.s
extern uint8_t prio_program[], prio_program_end[];

// In the order the program tries them; mailbox[i + 1] holds the answer.
static const struct {
    const char *call;
    const char *without_check;
} tries[] = {
    { "setpriority(me, 1000)",       "beyond every level: owns the CPU forever" },
    { "setpriority(me, -500)",       "a nice value that does not exist" },
    { "setpriority(me, -20)",        "a user jumps ahead of everyone" },
    { "setpriority(2, 19)",          "the shell (kernel_main) slowed for everybody" },
    { "setpriority(1, -20)",         "the idle task competes with real work" },
    { "sched_setscheduler(FIFO 99)", "real-time: runs before every normal process" },
};
#define TRY_COUNT (sizeof(tries) / sizeof(tries[0]))

static int simulate(void) {
    for (unsigned i = 0; i < TRY_COUNT; i++)
        ui_printf("  %-28s %s\n", tries[i].call, tries[i].without_check);
    return TRY_COUNT;
}

static int launch(void) {
    volatile uint32_t *mailbox = usertask_mailbox();

    int pid = usertask_start("prio", prio_program, prio_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the ring 3 program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (mailbox[0] != DONE) {
        ui_printf("  the program did not finish\n");
        return 0;
    }

    int accepted = 0;
    for (unsigned i = 0; i < TRY_COUNT; i++) {
        int ok = mailbox[i + 1] == 0;
        accepted += ok;
        ui_printf("  %-28s ", tries[i].call);
        ui_color(ok ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", ok ? "ACCEPTED" : "refused (-1)");
        ui_reset();
    }
    return accepted;
}

static const attack_t attack = {
    "priority_abuse",
    "ring 3 asks for nice 1000 / -20, renices others, or SCHED_FIFO 99",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // refused calls change nothing
    "priority_bounds"
};

void atk_priority_abuse_register(void) {
    registry_add_attack(&attack);
}
