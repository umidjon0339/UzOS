/*
 * atk_thread_exhaustion.c - ATTACK thread_exhaustion: one program takes
 * every process slot with threads.
 *
 * Breaks:     resource - 64 process slots for the whole system
 * Stopped by: thread_quota (defence/thread_quota)
 *
 * Every thread is a process slot and a kernel stack. A program that
 * starts threads in a loop (on purpose, or by a bug) fills the table;
 * slot_quota stops a normal user at 8, but a program run by root is
 * exempt from it.
 *
 * simulate(): explain.
 * launch():   run thread_exhaustion.s as user 1 in ring 3: 10 clone()
 *             calls with CLONE_THREAD. findings = threads beyond the
 *             cap (3 besides the main thread).
 * cleanup():  kill whatever is left of it.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"    // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"      // process_kill, process_get_table
#include "modules/process_management/security/thread_exhaustion/defence/thread_quota/def_thread_quota.h"  // THREADS_PER_GROUP
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500

// From thread_exhaustion.s
extern uint8_t threads_program[], threads_program_end[];

static int simulate(void) {
    ui_printf("  clone(CLONE_THREAD) in a loop: every thread is a process slot\n");
    ui_printf("  run by root, slot_quota does not apply: the table fills up\n");
    return 1;
}

static int launch(void) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start("threads", threads_program, threads_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != 1) {
        ui_printf("  the program did not finish\n");
        return 0;
    }
    int started = (int)box[1];
    int extra = started > THREADS_PER_GROUP - 1 ? started - (THREADS_PER_GROUP - 1) : 0;
    ui_printf("  10 tries: %d threads started besides the main one\n", started);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "more than the cap: the table can be filled"
                              : "refused after 3: one program, at most 4 threads");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            strcmp(table[i].name, "threads") == 0)
            killed += process_kill(table[i].pid);
    return killed;
}

static const attack_t attack = {
    "thread_exhaustion",
    "clone(CLONE_THREAD) in a loop until the process table is full",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "thread_quota"
};

void atk_thread_exhaustion_register(void) { registry_add_attack(&attack); }
