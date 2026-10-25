/*
 * atk_cpu_hog.c - ATTACK cpu_hog: a busy loop at the top priority.
 *
 * Breaks:     liveness - no process may starve the others
 * Stopped by: aging (security/defence/aging)
 *
 * With strict priorities, a process at level 7 that never sleeps is
 * always the best choice. Everything below it, the shell included,
 * would never run again. (It needs root to GET level 7, see
 * priority_bounds; but root programs have bugs too.)
 *
 * simulate(): show the arithmetic: how long a low process waits.
 * launch():   start "hog" at level 7 (spins) and "victim" at level 1
 *             (counts its turns), let the shell sleep 1 s, then see who
 *             got the CPU. The hog is killed before the results print.
 * cleanup():  kill any hog or victim still alive.
 *
 * Uses: process_create / process_kill (process_management), priority_set
 *       (core/priority), sys_sleep / sys_yield, timer_uptime_ms.
 */

#include "registry.h"
#include "process.h"
#include "priority.h"
#include "def_aging.h"
#include "sys_sleep.h"
#include "sys_yield.h"
#include "timer.h"
#include "string.h"
#include "ui.h"

#define ATTACKER_UID 1
#define MEASURE_MS   1000
#define SHELL_LEVEL  PRIORITY_DEFAULT

static volatile uint32_t victim_turns;

// Never sleeps, never yields: only the timer can take the CPU away.
static void hog(void) {
    while (1)
        ;
}

// One count per turn on the CPU, then give the rest of the turn away.
static void victim(void) {
    while (1) {
        victim_turns++;
        sys_yield();
    }
}

// Ticks a process at 'base' waits before aging ties it with level 7.
static uint32_t wait_ticks(int base) {
    return (uint32_t)(PRIORITY_MAX - base) * AGING_TICKS;
}

static int simulate(void) {
    uint32_t ms_per_tick = 1000 / TIMER_HZ;
    ui_printf("  hog at %d spins forever; victim at %d, shell at %d\n",
              PRIORITY_MAX, PRIORITY_MIN, SHELL_LEVEL);
    ui_printf("  strict priority: victim and shell never run again (OS frozen)\n");
    ui_printf("  with aging: victim waits %u ms, shell %u ms, then gets a turn\n",
              wait_ticks(PRIORITY_MIN) * ms_per_tick, wait_ticks(SHELL_LEVEL) * ms_per_tick);
    return 1;
}

static int start(const char *name, void (*entry)(void), int level) {
    int pid = process_create(name, entry, ATTACKER_UID);
    if (pid >= 0)
        priority_set(0, pid, level);    // the kernel sets it, as root
    return pid;
}

static int launch(void) {
    victim_turns = 0;
    int victim_pid = start("victim", victim, PRIORITY_MIN);
    int hog_pid    = start("hog", hog, PRIORITY_MAX);
    if (victim_pid < 0 || hog_pid < 0) {
        ui_printf("  could not start the processes\n");
        return 0;
    }

    uint32_t before = timer_uptime_ms();
    sys_sleep(MEASURE_MS);              // the shell, too, must get the CPU back
    uint32_t woke = timer_uptime_ms() - before;
    process_kill(hog_pid);              // stop the hog first, then report
    uint32_t turns = victim_turns;
    process_kill(victim_pid);

    ui_printf("  hog PID %d at level %d, victim PID %d at level %d\n",
              hog_pid, PRIORITY_MAX, victim_pid, PRIORITY_MIN);
    ui_printf("  victim got the CPU %u times in %u ms\n", turns, MEASURE_MS);
    ui_printf("  the shell slept %u ms, got the CPU back after %u ms\n", MEASURE_MS, woke);
    return turns == 0;                  // 1 finding = the victim starved
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        if (strcmp(p->name, "hog") == 0 || strcmp(p->name, "victim") == 0)
            killed += process_kill(p->pid);
    }
    return killed;
}

static const attack_t attack = {
    "cpu_hog",
    "a busy loop at priority 7 starves every process below it",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, cleanup,
    "aging"
};

void atk_cpu_hog_register(void) {
    registry_add_attack(&attack);
}
