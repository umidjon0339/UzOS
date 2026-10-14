#include "proc_security.h"
#include "process.h"
#include "timer.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: zombie leak
 * ===================================================================
 * Break INV_RESOURCE: process slots and their 4 KB stacks are finite.
 *
 * A process that returns from its entry point is dead, but the old
 * process_exit() halted forever, so its slot and stack never came back.
 * Enough of them and the table drains with nothing apparently running.
 * The reaper is the fix, and it runs from the timer tick.
 * =================================================================== */

#define LEAK_WORKERS 4    /* a live run leaks at most this many    */
#define LEAK_WAIT    20   /* ticks, enough for the reaper to run   */

/* Returning at once sends control to process_exit(): we become a zombie. */
static void leak_worker(void) {
}

static int count_state(process_state_t state) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state == state) n++;
    return n;
}

/* simulate() only reads the table - no process is touched at all. */
static int atk_zombie_leak_simulate(void) {
    vga_print_str("  free slots now         : ");
    vga_print_int(count_state(PROCESS_STATE_FREE));
    vga_print_str("\n  zombie slots held now  : ");
    vga_print_int(count_state(PROCESS_STATE_ZOMBIE));
    vga_print_str("\n  a live run would leak  : ");
    vga_print_int(LEAK_WORKERS);
    vga_print_str(" slots, 4 KB of stack each\n");
    vga_print_str("  (table read only - nothing was changed)\n");
    return LEAK_WORKERS;
}

/* launch() creates real workers that exit as soon as they run. */
static int atk_zombie_leak_launch(void) {
    int before = count_state(PROCESS_STATE_FREE);
    int made = 0;

    for (int i = 0; i < LEAK_WORKERS; i++) {
        /* uid 1 = unprivileged, so the normal user rules apply. */
        if (process_create("leak", leak_worker, 1) < 0) break;
        made++;
    }

    /* Let the workers run, return and die. The reaper is called from
     * the timer interrupt, so waiting here is what lets it clean up. */
    timer_wait(LEAK_WAIT);

    vga_print_str("  leaked ");
    vga_print_int(made);
    vga_print_str(" slot(s) by exiting\n  free slots before/after : ");
    vga_print_int(before);
    vga_print_str(" / ");
    vga_print_int(count_state(PROCESS_STATE_FREE));
    vga_print_str("\n  the reaper gave the slots back\n");
    return made;
}

/* Only the reaper may free a stack, so cleanup just waits and counts. */
static int atk_zombie_leak_cleanup(void) {
    int before = count_state(PROCESS_STATE_ZOMBIE);
    timer_wait(LEAK_WAIT);
    return before - count_state(PROCESS_STATE_ZOMBIE);
}

static const attack_t atk = {
    "zombie_leak",
    "an exited process never gives back its slot or its stack",
    INV_RESOURCE,
    SEV_HIGH,
    atk_zombie_leak_simulate,
    atk_zombie_leak_launch,
    atk_zombie_leak_cleanup,
    "reaper"
};

void atk_zombie_leak_register(void) {
    proc_security_register_attack(&atk);
}
