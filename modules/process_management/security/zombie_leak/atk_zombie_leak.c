/*
 * atk_zombie_leak.c - ATTACK zombie_leak: finished processes keep their slots.
 *
 * Breaks:     resource - slots and 4 KiB stacks must come back
 * Stopped by: reaper (defence/reaper)
 *
 * A process that returns becomes a zombie: it never runs again but still
 * holds its slot and stack. If nobody frees them, enough short-lived
 * processes drain the table while nothing seems to be running.
 *
 * simulate(): count free and zombie slots.
 * launch():   start LEAK_WORKERS processes that end at once, wait a
 *             little, and compare the free slots before and after.
 * cleanup():  wait for the reaper and count what it freed.
 *
 * Uses: process_create (core/process), timer_wait (device_management/timer).
 */

#include "modules/security_management/core/registry/registry.h"  // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"     // process_state_t, process_t
#include "modules/device_management/timer/timer.h"               // timer_wait
#include "modules/device_management/vga/vga.h"                   // vga_print_str, vga_print_int

#define LEAK_WORKERS 4
#define WAIT_TICKS   20         // 0.2 s: enough for the workers to end and be reaped
#define ATTACKER_UID 1

static void short_lived_worker(void) {
    // Returns at once -> process_exit() -> zombie.
}

static int count_state(process_state_t state) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state == state)
            n++;
    return n;
}

static int simulate(void) {
    vga_print_str("  free slots now   : ");
    vga_print_int(count_state(PROCESS_STATE_FREE));
    vga_print_str("\n  zombie slots now : ");
    vga_print_int(count_state(PROCESS_STATE_ZOMBIE));
    vga_print_str("\n  a live run leaks ");
    vga_print_int(LEAK_WORKERS);
    vga_print_str(" slots if nobody reaps them\n");
    return LEAK_WORKERS;
}

static int launch(void) {
    int before = count_state(PROCESS_STATE_FREE);
    int made = 0;

    for (int i = 0; i < LEAK_WORKERS; i++)
        if (process_create("leak", short_lived_worker, ATTACKER_UID) >= 0)
            made++;

    timer_wait(WAIT_TICKS);     // workers run, end, and get reaped

    int after = count_state(PROCESS_STATE_FREE);
    vga_print_str("  started ");
    vga_print_int(made);
    vga_print_str(" workers that ended at once\n  free slots before / after : ");
    vga_print_int(before);
    vga_print_str(" / ");
    vga_print_int(after);
    vga_print_str(after == before ? "  (all given back)\n" : "  (LEAKED)\n");
    return before - after;
}

static int cleanup(void) {
    int zombies = count_state(PROCESS_STATE_ZOMBIE);
    timer_wait(WAIT_TICKS);
    return zombies - count_state(PROCESS_STATE_ZOMBIE);
}

static const attack_t attack = {
    "zombie_leak",
    "finished processes never give back their slot or stack",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "reaper"
};

void atk_zombie_leak_register(void) {
    registry_add_attack(&attack);
}
