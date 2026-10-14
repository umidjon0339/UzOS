#include "proc_security.h"
#include "process.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: fork bomb
 * ===================================================================
 * Break the INV_RESOURCE guarantee: process slots are finite.
 *
 * A classic fork bomb calls fork() in a loop. Every child also runs
 * the loop, so the process count doubles until the table is full.
 * Once the table is full nothing else on the system can start - not
 * even a login shell to clean it up.
 *
 * The real thing is unbounded. To stay teachable we run it in fixed
 * rounds (ATTACK_BUDGET) and never fork from a child.
 * =================================================================== */

#define ATTACK_BUDGET 16   /* processes one live run may try to create */

static void bomb_worker(void) {
    while (1) {
        asm volatile("hlt");
    }
}

static int count_free_slots(void) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state == PROCESS_STATE_FREE) n++;
    return n;
}

static int same_word(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* simulate() reads the process table and changes nothing at all. */
static int atk_fork_bomb_simulate(void) {
    int free_slots = count_free_slots();

    vga_print_str("  free process slots now : ");
    vga_print_int(free_slots);
    vga_print_str("\n  a fork bomb would take : ");
    vga_print_int(free_slots);
    vga_print_str("\n  free slots left after  : 0\n");
    vga_print_str("  (nothing was created - simulate() is read-only)\n");
    return free_slots;
}

/* launch() is the real attack, capped so the demo stays usable. */
static int atk_fork_bomb_launch(void) {
    int made = 0;

    while (made < ATTACK_BUDGET) {
        /* uid 1 = unprivileged, so the defences apply to us. */
        if (process_create("bomb", bomb_worker, 1) < 0) break;
        made++;
    }

    vga_print_str("  created ");
    vga_print_int(made);
    vga_print_str(" unprivileged process(es)\n");
    return made;
}

/* cleanup() removes what we made, so the next test starts clean.
 * PIDs are collected first because killing changes the table we are
 * walking. */
static int atk_fork_bomb_cleanup(void) {
    process_t *t = process_get_table();
    uint32_t victims[MAX_PROCESSES];
    int n = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (t[i].state != PROCESS_STATE_FREE && same_word(t[i].name, "bomb"))
            victims[n++] = t[i].pid;
    }
    for (int i = 0; i < n; i++) process_kill(victims[i]);

    vga_print_str("  reaped ");
    vga_print_int(n);
    vga_print_str(" leftover process(es)\n");
    return n;
}

static const attack_t atk = {
    "fork_bomb",
    "fork() in a loop until every process slot is used",
    INV_RESOURCE,
    SEV_CRITICAL,
    atk_fork_bomb_simulate,
    atk_fork_bomb_launch,
    atk_fork_bomb_cleanup,
    "slot_quota"
};

void atk_fork_bomb_register(void) {
    proc_security_register_attack(&atk);
}
