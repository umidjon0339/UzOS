/*
 * atk_user_exception.c - ATTACK user_exception: crash the OS from ring 3.
 *
 * Breaks:     isolation - one program's crash must not stop the others
 * Stopped by: fault_isolation (security/defence/fault_isolation)
 *
 * A user program needs no special rights to make the CPU raise an
 * exception: dividing by zero is enough. If the kernel answers every
 * exception with "System Halted!", every user is one instruction away
 * from stopping the whole machine.
 *
 * simulate(): list the five programs and what each one would cause.
 * launch():   run them, one after the other, as user 1 in ring 3
 *             (user_exception.s). Each must be killed; the shell must
 *             still be here afterwards.
 * cleanup():  kill any of them that is somehow still running.
 *
 * Uses: usertask_start / usertask_wait (process_management/core/usertask),
 *       cpu_fault_total / cpu_fault_log_get (core/faults),
 *       isr_exception_name (arch/x86/isr), process_kill (core/process).
 */

#include "registry.h"
#include "usertask.h"
#include "cpu_faults.h"
#include "process.h"
#include "string.h"
#include "ui.h"

#define ATTACKER_UID 1
#define WAIT_MS      300        // each program dies within a tick or two

// From user_exception.s
extern uint8_t uexc_div0[], uexc_div0_end[];
extern uint8_t uexc_ud2[],  uexc_ud2_end[];
extern uint8_t uexc_cli[],  uexc_cli_end[];
extern uint8_t uexc_in[],   uexc_in_end[];
extern uint8_t uexc_hlt[],  uexc_hlt_end[];

static const struct {
    const char *name;           // also the process name
    const uint8_t *start, *end;
    uint8_t vector;             // the exception it should raise
} programs[] = {
    { "div0", uexc_div0, uexc_div0_end,  0 },
    { "ud2",  uexc_ud2,  uexc_ud2_end,   6 },
    { "cli",  uexc_cli,  uexc_cli_end,  13 },
    { "in",   uexc_in,   uexc_in_end,   13 },
    { "hlt",  uexc_hlt,  uexc_hlt_end,  13 },
};
#define PROGRAM_COUNT (sizeof(programs) / sizeof(programs[0]))

static int simulate(void) {
    for (unsigned i = 0; i < PROGRAM_COUNT; i++)
        ui_printf("  %-5s raises #%-2u %-26s -> no handler: System Halted\n",
                  programs[i].name, programs[i].vector,
                  isr_exception_name(programs[i].vector));
    return PROGRAM_COUNT;
}

// Run one program and report what became of it. 1 = it was killed by a fault.
static int run_one(unsigned i) {
    uint32_t before = cpu_fault_total();
    int pid = usertask_start(programs[i].name, programs[i].start, programs[i].end,
                             ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  %-5s could not start\n", programs[i].name);
        return 0;
    }

    int ended  = usertask_wait(pid, WAIT_MS);
    int killed = cpu_fault_total() > before;
    const cpu_fault_t *f = killed ? cpu_fault_log_get(0) : 0;

    ui_printf("  %-5s ", programs[i].name);
    if (f) {
        ui_color(UI_GOOD);
        ui_printf("#%u %s: PID %d killed, kernel still running\n",
                  f->vector, isr_exception_name(f->vector), pid);
    } else {
        ui_color(UI_BAD);
        ui_printf("%s\n", ended ? "the CPU let it through!" : "still running");
    }
    ui_reset();
    return killed;
}

static int launch(void) {
    int killed = 0;
    for (unsigned i = 0; i < PROGRAM_COUNT; i++)
        killed += run_one(i);
    ui_printf("  %d of %d programs faulted; you are reading this, so the OS survived\n",
              killed, (int)PROGRAM_COUNT);
    return killed;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        for (unsigned j = 0; j < PROGRAM_COUNT; j++)
            if (strcmp(p->name, programs[j].name) == 0)
                killed += process_kill(p->pid);
    }
    return killed;
}

static const attack_t attack = {
    "user_exception",
    "ring 3 runs div 0, ud2, cli, in, hlt: each is a CPU exception",
    INV_ISOLATION, SEV_CRITICAL,
    simulate, launch, cleanup,
    "fault_isolation"
};

void atk_user_exception_register(void) {
    registry_add_attack(&attack);
}
