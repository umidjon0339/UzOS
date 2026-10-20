/*
 * atk_setuid_escalation.c - ATTACK setuid_escalation: a user asks to become root.
 *
 * Breaks:     privilege - a normal user cannot give itself root's rights
 * Stopped by: setuid_guard (security/defence/setuid_guard)
 *
 * setuid(uid) exists so that root can GIVE UP its rights. If it did not
 * check who is asking, any program could call setuid(0) and be root.
 *
 * simulate(): explain what setuid(0) would give a guest program.
 * launch():   run setuid_escalation.s as guest in ring 3; it calls
 *             setuid(0) and getuid() and leaves the answers in the
 *             usertask mailbox.
 *
 * Uses: usertask_* (process_management/core/usertask), and the syscalls
 *       setuid / getuid (syscalls/) through int 0x80.
 */

#include "security_lab.h"
#include "usertask.h"
#include "users.h"
#include "ui.h"

#define WAIT_MS 300
#define DONE    0x600D          // what the program writes to mailbox[0]

// From setuid_escalation.s
extern uint8_t setuid_program[], setuid_program_end[];

static int simulate(void) {
    ui_printf("  guest program: setuid(0), then getuid()\n");
    ui_printf("  without a check: getuid() = 0 - it may now kill any process,\n");
    ui_printf("  read any file and set any priority: every check trusts the uid\n");
    return 1;
}

static int launch(void) {
    volatile uint32_t *mailbox = usertask_mailbox();

    int pid = usertask_start("escalate", setuid_program, setuid_program_end, UID_GUEST);
    if (pid < 0) {
        ui_printf("  could not start the ring 3 program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (mailbox[0] != DONE) {
        ui_printf("  the program did not finish\n");
        return 0;
    }

    int became_root = mailbox[2] == UID_ROOT;
    ui_printf("  setuid(0) returned %d, getuid() is now %u  ",
              (int)mailbox[1], mailbox[2]);
    ui_color(became_root ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", became_root ? "ROOT!" : "(still guest)");
    ui_reset();
    return became_root;
}

static const attack_t attack = {
    "setuid_escalation",
    "a guest program in ring 3 calls setuid(0) to become root",
    INV_PRIVILEGE, SEV_CRITICAL,
    simulate, launch, 0,        // a refused call changes nothing
    "setuid_guard"
};

void atk_setuid_escalation_register(void) {
    proc_security_register_attack(&attack);
}
