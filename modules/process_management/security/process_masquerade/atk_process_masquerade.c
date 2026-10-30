/*
 * atk_process_masquerade.c - ATTACK process_masquerade: pose as the kernel.
 *
 * Breaks:     trust - `ps` must show who a process really is
 * Stopped by: name_guard (defence/name_guard)
 *
 * People and tools find processes by name. A program that renames
 * itself "kernel_main" looks like part of the OS, so nobody kills it.
 * A name holding an escape sequence repaints the screen of whoever
 * runs `ps` (here: ESC [2J clears it).
 *
 * simulate(): list the three renames.
 * launch():   run process_masquerade.s as user 1 in ring 3. It asks
 *             prctl(PR_SET_NAME) for each. findings = harmful renames
 *             the kernel accepted (the normal one must work).
 * cleanup():  kill it if it is somehow still running.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), ui_* (ui).
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"    // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"      // process_kill, process_get_table
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500

// From process_masquerade.s
extern uint8_t masq_program[], masq_program_end[];

static const char *const tries[] = {
    "rename to \"kernel_main\"",
    "rename to ESC[2J (clear screen)",
    "rename to \"masq_ok\" (normal)",
};

static int simulate(void) {
    for (int i = 0; i < 3; i++)
        ui_printf("  prctl(PR_SET_NAME): %s\n", tries[i]);
    return 2;
}

static int launch(void) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start("masquerade", masq_program, masq_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);

    int harmful = 0;
    for (int i = 0; i < 3; i++) {
        int accepted = (int32_t)box[i + 1] == 0;
        int bad = i < 2 ? accepted : !accepted;     // the normal one SHOULD work
        harmful += i < 2 && accepted;
        ui_printf("  %-34s ", tries[i]);
        ui_color(bad ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", accepted ? "accepted" : "refused");
        ui_reset();
    }
    char name[5] = { 0 };
    memcpy(name, (const void *)&box[4], 4);
    ui_printf("  its name in the end starts with \"%s\"\n", name);
    return harmful;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
            (strcmp(p->name, "masquerade") == 0 || strcmp(p->name, "masq_ok") == 0))
            killed += process_kill(p->pid);
    }
    return killed;
}

static const attack_t attack = {
    "process_masquerade",
    "a program renames itself \"kernel_main\", or to an escape sequence",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "name_guard"
};

void atk_process_masquerade_register(void) { registry_add_attack(&attack); }
