/*
 * atk_priority_out_of_range.c - ATTACK priority_out_of_range: ask for a level you may not have.
 *
 * Breaks:     trust - a number from a user program is checked before use
 * Stopped by: priority_bounds (security/defence/priority_bounds)
 *
 * setpriority(pid, level) takes both numbers from the caller. Without
 * checks a user program could:
 *   - pick a level no process should have (1000, -5)
 *   - raise itself to the top and push everyone else back
 *   - lower the shell, or change the idle task, slowing the whole OS
 *
 * simulate(): list the five calls and what each one would do.
 * launch():   run priority_out_of_range.s as user 1 in ring 3; it makes
 *             the five calls for real and leaves the answers in the
 *             usertask mailbox.
 *
 * Uses: usertask_* (process_management/core/usertask), and the syscall
 *       setpriority (syscalls/setpriority) through int 0x80.
 */

#include "security_lab.h"
#include "usertask.h"
#include "ui.h"

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
    { "setpriority(me, 1000)", "above every level and the aging cap: owns the CPU" },
    { "setpriority(me, -5)",   "below idle; 4 billion if read as unsigned" },
    { "setpriority(me, 7)",    "a user jumps ahead of everyone" },
    { "setpriority(2, 1)",     "the shell (kernel_main) slowed for everybody" },
    { "setpriority(1, 7)",     "the idle task competes with real work" },
};
#define TRY_COUNT (sizeof(tries) / sizeof(tries[0]))

static int simulate(void) {
    for (unsigned i = 0; i < TRY_COUNT; i++)
        ui_printf("  %-22s %s\n", tries[i].call, tries[i].without_check);
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
        ui_printf("  %-22s ", tries[i].call);
        ui_color(ok ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", ok ? "ACCEPTED" : "refused (-1)");
        ui_reset();
    }
    return accepted;
}

static const attack_t attack = {
    "priority_out_of_range",
    "ring 3 asks setpriority() for 1000, -5, 7, or for someone else",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // refused calls change nothing
    "priority_bounds"
};

void atk_priority_out_of_range_register(void) {
    proc_security_register_attack(&attack);
}
