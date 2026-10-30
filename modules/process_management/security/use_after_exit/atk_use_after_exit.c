/*
 * atk_use_after_exit.c - ATTACK use_after_exit: act on a process slot
 * after its owner is gone and a newcomer took it.
 *
 * Breaks:     consistency - a reference must name one process, for ever
 * Stopped by: pcb_generation (defence/pcb_generation)
 *
 * The kernel keeps processes in 64 reusable slots. Code that keeps a
 * process_t * while it sleeps (a wait, a pidfd, a debugger) may wake up
 * after that process ended and the reaper gave its slot to a new one.
 * Using the old pointer now signals, inspects or reaps the NEWCOMER.
 *
 * simulate(): explain the steps.
 * launch():   1. start "victim" (user 1), keep a reference to it
 *             2. end it, let the reaper free the slot
 *             3. start "newcomer" (root) until it lands in the same slot
 *             4. use the old reference: the raw pointer would reach the
 *                newcomer; def_pcb_ref_get() must refuse.
 *             findings = 1 if the old reference reached the newcomer.
 * cleanup():  kill both.
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       def_reap_zombies (security/zombie_leak/defence/reaper),
 *       def_pcb_ref / def_pcb_ref_get (its defence), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/zombie_leak/defence/reaper/def_reaper.h"  // def_reap_zombies
#include "modules/process_management/security/use_after_exit/defence/pcb_generation/def_pcb_generation.h"  // def_pcb_ref, def_pcb_ref_get
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define TRIES 8                 // newcomers to start until one gets the slot

static int victim_pid = -1, newcomer_pid = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  keep a pointer to a process, it ends, its slot is reused\n");
    ui_printf("  the old pointer now reaches the NEWCOMER\n");
    return 1;
}

static int launch(void) {
    victim_pid = process_create("victim", sleeper, 1);
    process_t *victim = victim_pid > 0 ? process_get_by_pid(victim_pid) : 0;
    if (!victim) {
        ui_printf("  could not start the victim\n");
        return 0;
    }
    pcb_ref_t ref = def_pcb_ref(victim);
    process_t *raw = victim;                        // what careless code keeps
    ui_printf("  victim PID %d in slot %x\n", victim_pid, (uint32_t)raw);

    process_kill(victim_pid);
    def_reap_zombies();                             // its slot is free again

    for (int i = 0; i < TRIES && raw->state == PROCESS_STATE_FREE; i++)
        newcomer_pid = process_create("newcomer", sleeper, 0);
    if (raw->state == PROCESS_STATE_FREE) {
        ui_printf("  the slot was not reused (try again)\n");
        return 0;
    }
    ui_printf("  newcomer PID %u (root) took the same slot\n", raw->pid);
    ui_printf("  raw pointer now names: %s (PID %u)\n", raw->name, raw->pid);

    process_t *got = def_pcb_ref_get(&ref);
    ui_printf("  checked reference:     ");
    ui_color(got ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", got ? "reaches the newcomer!" : "refused: the victim is gone");
    ui_reset();
    return got != 0;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
            (strcmp(p->name, "victim") == 0 || strcmp(p->name, "newcomer") == 0))
            killed += process_kill(p->pid);
    }
    victim_pid = newcomer_pid = -1;
    return killed;
}

static const attack_t attack = {
    "use_after_exit",
    "keep a process pointer across its death; reach the slot's newcomer",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, cleanup,
    "pcb_generation"
};

void atk_use_after_exit_register(void) { registry_add_attack(&attack); }
