/*
 * atk_signal_flood.c - ATTACK signal_flood: fill the kernel's signal queue.
 *
 * Breaks:     resource - the queued-signal pool is shared by everyone
 * Stopped by: sigqueue_limit (defence/sigqueue_limit)
 *
 * rt_sigqueueinfo() sends a signal WITH a value, so the kernel keeps
 * every one until it is delivered (core/signal/sigqueue.c, 64 entries
 * for all processes). If the target blocks the signal, it is never
 * delivered: the entries stay. One user queueing to such a target can
 * take the whole pool, and then no one else can queue a signal.
 *
 * simulate(): show the pool and what one user could take.
 * launch():   as user 1, start a target that never takes its signals,
 *             then queue 64 signals to it. With the defence, 16 get in.
 *             findings = entries taken beyond the user's share.
 * cleanup():  kill the target; the reaper frees its queued entries.
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       sigqueue_add / sigqueue_count_user (core/signal), sys_sleep
 *       (syscalls/sleep), ui_* (ui).
 */

#include "modules/security_management/core/registry/registry.h"                                          // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"                                             // process_create, process_kill
#include "modules/process_management/core/signal/signal.h"                                               // sigqueue_add, SIGUSR1
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                                         // sys_sleep
#include "modules/process_management/security/signal_flood/defence/sigqueue_limit/def_sigqueue_limit.h"  // SIGQUEUE_USER_MAX
#include "modules/ui/core/style/ui.h"                                                                    // ui_printf, ui_color

#define ATTACKER_UID 1
#define TRIES        SIGQUEUE_SIZE      // try to take every entry

static int target_pid = -1;

// The target: a ring 0 process. Ring 0 never has signals delivered, so
// whatever is queued for it stays until it ends.
static void target(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  the pool holds %d queued signals for ALL processes\n", SIGQUEUE_SIZE);
    ui_printf("  a target that blocks the signal never takes them back\n");
    ui_printf("  user %d could queue %d to it: nobody else could queue one\n",
              ATTACKER_UID, SIGQUEUE_SIZE);
    return 1;
}

static int launch(void) {
    target_pid = process_create("flood_target", target, ATTACKER_UID);
    process_t *t = target_pid > 0 ? process_get_by_pid(target_pid) : 0;
    if (!t) {
        ui_printf("  could not start the target\n");
        return 0;
    }

    int queued = 0;
    for (int i = 0; i < TRIES; i++) {
        k_siginfo_t info = { SIGUSR1, 0, SI_QUEUE, 0, ATTACKER_UID, (uint32_t)i };
        if (sigqueue_add(t, &info)) {
            signal_send_info(t, SIGUSR1, SI_QUEUE, (uint32_t)i);
            queued++;
        }
    }

    int extra = queued > SIGQUEUE_USER_MAX ? queued - SIGQUEUE_USER_MAX : 0;
    ui_printf("  user %d queued %d of %d; the pool keeps %d free for the others\n",
              ATTACKER_UID, queued, TRIES, SIGQUEUE_SIZE - sigqueue_count_user(ATTACKER_UID));
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "the pool is gone: no one else can queue a signal"
                              : "refused after its share: the pool stays usable");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    if (target_pid <= 0 || !process_get_by_pid(target_pid))
        return 0;
    int killed = process_kill(target_pid);
    target_pid = -1;
    return killed;
}

static const attack_t attack = {
    "signal_flood",
    "queue signals to a target that never takes them, until the pool is full",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "sigqueue_limit"
};

void atk_signal_flood_register(void) { registry_add_attack(&attack); }
