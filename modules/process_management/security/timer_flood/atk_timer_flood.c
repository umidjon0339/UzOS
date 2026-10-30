/*
 * atk_timer_flood.c - ATTACK timer_flood: make the timer tick work for you.
 *
 * Breaks:     resource - the kernel's time on every tick is shared
 * Stopped by: timer_limits (defence/timer_limits)
 *
 * Each process has three interval timers (REAL, VIRTUAL, PROF). Armed
 * with the shortest interval, each one makes the kernel count, signal,
 * wake and deliver 100 times a second. A user who arms all three in
 * every process they may have turns the timer interrupt into their
 * private worker.
 *
 * simulate(): count what a user could arm.
 * launch():   as user 1, start 4 processes and arm all three timers of
 *             each with a 10 ms interval, the way setitimer() does.
 *             findings = periodic timers armed beyond the limit of 8.
 * cleanup():  kill the processes (their timers go with them).
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       def_itimer_allowed (its defence), ITIMERS (core/signal), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/core/signal/signal.h"        // ITIMER_REAL
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/timer_flood/defence/timer_limits/def_timer_limits.h"  // def_itimer_allowed
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define TICKERS      4

static int pids[TICKERS];

// A sleeper; launch() blocks all its signals, so the timers fire harmlessly.
static void ticker(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  3 timers per process x 8 processes per user = 24 timers\n");
    ui_printf("  each at 100 Hz: 2400 signals a second for the kernel to send\n");
    return 1;
}

static int launch(void) {
    int armed = 0, tried = 0;
    for (int i = 0; i < TICKERS; i++) {
        pids[i] = process_create("ticker", ticker, ATTACKER_UID);
        process_t *p = pids[i] > 0 ? process_get_by_pid(pids[i]) : 0;
        if (!p)
            continue;
        p->sig_blocked = 0xFFFFFFFFu;               // the signals pile up, harmless
        for (int t = 0; t < ITIMERS; t++, tried++) {
            if (!def_itimer_allowed(p, t))
                continue;
            p->itimer_value[t] = p->itimer_interval[t] = 1;     // every tick
            armed++;
        }
    }
    int extra = armed > TIMER_USER_MAX ? armed - TIMER_USER_MAX : 0;
    ui_printf("  user %d armed %d of %d periodic timers\n", ATTACKER_UID, armed, tried);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "the timer tick now works for one user" : "refused after 8: the tick stays fair");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    int killed = 0;
    for (int i = 0; i < TICKERS; i++) {
        if (pids[i] > 0 && process_get_by_pid(pids[i]))
            killed += process_kill(pids[i]);
        pids[i] = 0;
    }
    return killed;
}

static const attack_t attack = {
    "timer_flood",
    "arm every interval timer of many processes at 100 Hz",
    INV_RESOURCE, SEV_LOW,
    simulate, launch, cleanup,
    "timer_limits"
};

void atk_timer_flood_register(void) { registry_add_attack(&attack); }
