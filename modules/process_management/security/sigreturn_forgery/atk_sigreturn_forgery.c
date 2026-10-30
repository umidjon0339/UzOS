/*
 * atk_sigreturn_forgery.c - ATTACK sigreturn_forgery: SROP to IOPL 3.
 *
 * Breaks:     privilege - ring 3 must never get I/O privilege
 * Stopped by: sigframe_check (defence/sigframe_check)
 *
 * When a signal handler returns, rt_sigreturn loads EVERY register from
 * the frame on the program's stack: memory the program owns. If the
 * kernel trusts it, a program can write any register values it likes,
 * EFLAGS included. With IOPL = 3 in EFLAGS, ring 3 may run cli and
 * in/out: it controls interrupts and every device.
 *
 * simulate(): explain the two tricks.
 * launch():   run both programs of sigreturn_forgery.s as user 1:
 *               srop_forged  calls rt_sigreturn with no signal at all
 *               srop_tamper  a real handler sets IOPL in its own frame
 *             findings = programs that ran cli from ring 3.
 * cleanup():  kill any of them still running.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), ui_* (ui).
 */

#include "modules/security_management/core/registry/registry.h"  // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"   // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"     // process_kill, process_get_table
#include "lib/string/string.h"                                   // strcmp
#include "modules/ui/core/style/ui.h"                            // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500
#define WON          0x10       // what a program writes to mailbox[1] if cli worked

// From sigreturn_forgery.s
extern uint8_t srop_forged[], srop_forged_end[];
extern uint8_t srop_tamper[], srop_tamper_end[];

static const struct {
    const char *name;
    const uint8_t *start, *end;
    const char *trick;
} programs[] = {
    { "srop_forged", srop_forged, srop_forged_end, "fake frame, no signal" },
    { "srop_tamper", srop_tamper, srop_tamper_end, "handler edits its frame" },
};
#define PROGRAM_COUNT (sizeof(programs) / sizeof(programs[0]))

static int simulate(void) {
    ui_printf("  rt_sigreturn loads every register from the program's own stack\n");
    ui_printf("  1. forge a frame and call rt_sigreturn with no signal\n");
    ui_printf("  2. let a real handler set IOPL 3 in its saved EFLAGS\n");
    ui_printf("  either way: ring 3 could run cli and in/out\n");
    return PROGRAM_COUNT;
}

static int run_one(unsigned i) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start(programs[i].name, programs[i].start, programs[i].end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  %-12s could not start\n", programs[i].name);
        return 0;
    }
    usertask_wait(pid, WAIT_MS);

    int won = box[1] == WON;
    ui_printf("  %-12s %-24s ", programs[i].name, programs[i].trick);
    ui_color(won ? UI_BAD : UI_GOOD);
    if (won)
        ui_printf("IOPL 3: cli ran in ring 3!\n");
    else if (box[2])
        ui_printf("came back with EFLAGS %x: IOPL 0\n", box[2]);
    else
        ui_printf("refused: PID %d ended (SIGSEGV)\n", pid);
    ui_reset();
    return won;
}

static int launch(void) {
    int won = 0;
    for (unsigned i = 0; i < PROGRAM_COUNT; i++)
        won += run_one(i);
    return won;
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
    "sigreturn_forgery",
    "SROP: rt_sigreturn with a forged or edited frame, to get IOPL 3",
    INV_PRIVILEGE, SEV_CRITICAL,
    simulate, launch, cleanup,
    "sigframe_check"
};

void atk_sigreturn_forgery_register(void) { registry_add_attack(&attack); }
