/*
 * atk_memory_peek.c - ATTACK memory_peek: read memory that is not yours.
 *
 * Breaks:     disclosure - a process's memory is its own
 * Stopped by: vm_access_check (defence/vm_access_check)
 *
 * process_vm_readv() copies memory out of another process in one call:
 * a debugger's read without stopping it. Without a check, any user could
 * read root's processes - or name a "remote" address that is really
 * the kernel and read the kernel itself.
 *
 * simulate(): list the reads.
 * launch():   run memory_peek.s as user 1 in ring 3 (see its header).
 *             findings = forbidden reads that returned data (the honest
 *             read of its own memory must work).
 * cleanup():  kill it if still running.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"    // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"      // process_kill, process_get_table
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500

// From memory_peek.s
extern uint8_t peek_program[], peek_program_end[];

static const char *const reads[] = {
    "readv(idle, 16 bytes)", "readv(me, kernel 0x00100000)", "readv(me, my own memory)",
};

static int simulate(void) {
    for (int i = 0; i < 3; i++)
        ui_printf("  process_vm_%s\n", reads[i]);
    return 2;
}

static int launch(void) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start("peeker", peek_program, peek_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != 1) {
        ui_printf("  the program did not finish\n");
        return 0;
    }
    int leaked = 0;
    for (int i = 0; i < 3; i++) {
        int32_t got = (int32_t)box[i + 1];
        int bad = i < 2 ? got > 0 : got != 16;
        leaked += i < 2 && got > 0;
        ui_printf("  process_vm_%-28s ", reads[i]);
        ui_color(bad ? UI_BAD : UI_GOOD);
        if (got > 0)
            ui_printf("%d bytes%s\n", got, i < 2 ? " read!" : " (its own: fine)");
        else
            ui_printf("refused\n");
        ui_reset();
    }
    return leaked;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            strcmp(table[i].name, "peeker") == 0)
            killed += process_kill(table[i].pid);
    return killed;
}

static const attack_t attack = {
    "memory_peek",
    "process_vm_readv from root's process, or from kernel addresses",
    INV_DISCLOSURE, SEV_HIGH,
    simulate, launch, cleanup,
    "vm_access_check"
};

void atk_memory_peek_register(void) { registry_add_attack(&attack); }
