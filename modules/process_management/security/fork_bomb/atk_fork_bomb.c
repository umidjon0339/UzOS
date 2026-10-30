/*
 * atk_fork_bomb.c - ATTACK fork_bomb: fill the process table.
 *
 * Breaks:     resource - there are only MAX_PROCESSES slots
 * Stopped by: slot_quota (defence/slot_quota)
 *
 * A real fork bomb calls fork() in a loop, and every child loops too,
 * until no slot is left and nothing else can start.
 *
 * simulate(): count the free slots a bomb would take.
 * launch():   as user 1, create up to ATTACK_BUDGET idle processes.
 * cleanup():  kill every process named "bomb".
 *
 * Uses: process_create / process_kill / process_get_table (core/process).
 */

#include "modules/security_management/core/registry/registry.h"  // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"     // process_count_free_slots, process_create
#include "lib/string/string.h"                                   // strcmp
#include "modules/device_management/vga/vga.h"                   // vga_print_str, vga_print_int

#define ATTACK_BUDGET 16        // most processes one live run may create
#define ATTACKER_UID  1         // a normal user, so the quota applies

static void bomb_worker(void) {
    while (1)
        asm volatile("hlt");
}

static int simulate(void) {
    int free_slots = process_count_free_slots();

    vga_print_str("  free process slots now : ");
    vga_print_int(free_slots);
    vga_print_str("\n  a fork bomb takes them all, leaving 0\n");
    return free_slots;
}

static int launch(void) {
    int made = 0;

    while (made < ATTACK_BUDGET && process_create("bomb", bomb_worker, ATTACKER_UID) >= 0)
        made++;

    vga_print_str("  created ");
    vga_print_int(made);
    vga_print_str(" of ");
    vga_print_int(ATTACK_BUDGET);
    vga_print_str(" processes as uid 1\n");
    return made;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
            strcmp(p->name, "bomb") == 0)
            killed += process_kill(p->pid);
    }

    vga_print_str("  killed ");
    vga_print_int(killed);
    vga_print_str(" bomb process(es)\n");
    return killed;
}

static const attack_t attack = {
    "fork_bomb",
    "create processes in a loop until every slot is used",
    INV_RESOURCE, SEV_CRITICAL,
    simulate, launch, cleanup,
    "slot_quota"
};

void atk_fork_bomb_register(void) {
    registry_add_attack(&attack);
}
