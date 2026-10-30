/*
 * atk_syscall_abuse.c - ATTACK syscall_abuse: a taken-over program uses
 * syscalls it never needed.
 *
 * Breaks:     privilege - a sandboxed program keeps only what it kept
 * Stopped by: seccomp_filter (defence/seccomp_filter)
 *
 * A program that handles untrusted input (an image, a web page) can be
 * taken over by it. The attacker's code then has every syscall the
 * program has: open your files, kill your processes, try setuid(0). A
 * program that put itself in a seccomp sandbox first leaves the
 * attacker only the handful of calls it really needs.
 *
 * simulate(): list what the injected code tries.
 * launch():   run syscall_abuse.s as user 1: it sets a seccomp filter,
 *             then "the attacker" calls open, kill, setuid; then strict
 *             mode and getpid. findings = calls that got through.
 * cleanup():  kill it if still running; remove the file if it was made.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), sys_unlink
 *       (filesystem_management), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"    // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"      // process_kill, process_get_table
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"  // sys_unlink
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500

// From syscall_abuse.s
extern uint8_t abuse_program[], abuse_program_end[];

static const char *const tries[] = {
    "open(\"/home/guest/stolen\")", "kill(1, SIGKILL)", "setuid(0)",
    "getpid() in strict mode",
};

static int simulate(void) {
    ui_printf("  a program reading untrusted input is taken over; its new code tries:\n");
    for (int i = 0; i < 4; i++)
        ui_printf("    %s\n", tries[i]);
    return 4;
}

static int launch(void) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start("sandboxed", abuse_program, abuse_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != 1) {
        ui_printf("  the program could not set up its sandbox\n");
        return 0;
    }
    int through = 0;
    for (int i = 0; i < 4; i++) {
        int got = i < 3 ? (int32_t)box[i + 1] >= 0 : box[4] == 1;
        through += got;
        ui_printf("  %-30s ", tries[i]);
        ui_color(got ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", got ? "went through!" : i < 3 ? "failed: -1 (filter)" : "ended by SIGSYS");
        ui_reset();
    }
    return through;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int done = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            strcmp(table[i].name, "sandboxed") == 0)
            done += process_kill(table[i].pid);
    done += sys_unlink("/home/guest/stolen") == 0;
    return done;
}

static const attack_t attack = {
    "syscall_abuse",
    "injected code in a sandboxed program calls open, kill, setuid",
    INV_PRIVILEGE, SEV_HIGH,
    simulate, launch, cleanup,
    "seccomp_filter"
};

void atk_syscall_abuse_register(void) { registry_add_attack(&attack); }
