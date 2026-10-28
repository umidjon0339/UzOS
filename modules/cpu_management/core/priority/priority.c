/*
 * priority.c - Priority scheduling with nice values, time slices, aging
 *              and real-time throttling.
 *
 * Runs inside the timer interrupt (priority_tick) and inside
 * scheduler_yield (priority_pick_next), so it only walks the process
 * table: 64 slots, a few comparisons each.
 *
 * Uses: process_get_table / process_get_by_pid (process_management),
 *       policy_* (core/policy), timer_frequency (device_management),
 *       ui_* (ui) for the one-time throttling message, and two defences:
 *         aging           - waiting raises the effective level
 *         priority_bounds - who may set which nice value
 */

#include "modules/cpu_management/core/priority/priority.h"  // this part
#include "modules/cpu_management/core/policy/policy.h"      // SCHED_*, policy_slice_ticks
#include "modules/device_management/timer/timer.h"          // timer_frequency
#include "modules/ui/core/style/ui.h"                       // ui_color, ui_printf
#include "modules/cpu_management/security/starvation/defence/aging/def_aging.h"
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"

#define RANK_REALTIME   100     // + rt_priority (1..99)
#define RANK_PER_LEVEL  10      // normal: effective level * 10
#define RANK_SCHED_IDLE 5       // above the idle task (0), below level 1 (10)

// RT throttling: ticks used by real-time processes in the current second.
static uint32_t rt_period_ticks, rt_used_ticks, rt_throttle_count;
static int rt_throttled;

// The usual nice of each level, index = level (0 = the idle task).
static const int nice_of_level[PRIORITY_MAX + 1] = { 19, 19, 13, 7, 0, -6, -13, -20 };

int priority_level_of_nice(int nice) {
    if (nice < NICE_MIN) nice = NICE_MIN;
    if (nice > NICE_MAX) nice = NICE_MAX;
    // 40 nice values onto 7 levels: 19 -> 1, 0 -> 4, -20 -> 7.
    return (NICE_MAX - nice) * PRIORITY_MAX / 40 + 1;
}

int priority_nice_of_level(int level) {
    if (level < PRIORITY_MIN) level = PRIORITY_MIN;
    if (level > PRIORITY_MAX) level = PRIORITY_MAX;
    return nice_of_level[level];
}

static int runnable(const process_t *p) {
    return p->state == PROCESS_STATE_READY || p->state == PROCESS_STATE_RUNNING;
}

int priority_effective(const process_t *p) {
    // DEFENCE aging: the longer a process waits, the higher it ranks.
    return def_aging_effective(p->priority, p->waited_ticks);
}

// The rank with or without aging (aging = 0: the level it was given).
static int rank_of(const process_t *p, int aging) {
    if (policy_is_realtime(p->policy) && !rt_throttled)
        return RANK_REALTIME + p->rt_priority;
    if (p->policy == SCHED_IDLE)
        return RANK_SCHED_IDLE;
    return (aging ? priority_effective(p) : p->priority) * RANK_PER_LEVEL;
}

int priority_rank(const process_t *p) {
    return rank_of(p, 1);
}

// Rules 1 and 2. *rescued = 1 if the winner only won thanks to aging:
// its own rank is below another runnable process's.
static process_t *choose(process_t *current, int *rescued) {
    process_t *table = process_get_table();
    int start = current ? current - table : 0;
    process_t *best = 0;
    int best_rank = -1, top_base = -1;

    // Start AFTER current and end with current itself, so among equal
    // ranks everyone else comes first: that is the round robin.
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        process_t *p = &table[(start + i) % MAX_PROCESSES];
        if (!runnable(p))
            continue;
        int rank = rank_of(p, 1);
        if (rank > best_rank) {             // strictly better: ties keep the earlier one
            best = p;
            best_rank = rank;
        }
        if (rank_of(p, 0) > top_base)
            top_base = rank_of(p, 0);
    }

    *rescued = best && rank_of(best, 0) < top_base;
    return best;
}

process_t *priority_pick_next(process_t *current) {
    int rescued;
    process_t *next = choose(current, &rescued);

    // DEFENCE aging: without it this process would have starved.
    // Say so (at most once a second).
    if (rescued)
        def_aging_note_rescue();
    return next;
}

// Count real-time ticks; at 95% of a second, throttle until it is over.
// Returns 1 if throttling just started (the RT process must stop now).
static int rt_throttle_tick(const process_t *current) {
    uint32_t hz = timer_frequency();
    int started = 0;

    if (current && current->state == PROCESS_STATE_RUNNING &&
        policy_is_realtime(current->policy) && !rt_throttled)
        rt_used_ticks++;

    if (++rt_period_ticks >= hz) {          // a new second: real-time may run again
        rt_period_ticks = rt_used_ticks = 0;
        rt_throttled = 0;
    } else if (!rt_throttled && rt_used_ticks >= hz * RT_RUNTIME_PERCENT / 100) {
        rt_throttled = 1;
        started = 1;
        if (rt_throttle_count++ == 0) {     // like Linux: say it once
            uint8_t color = vga_get_color();
            ui_color(UI_WARNING);
            ui_printf("\n[sched] RT throttling activated (real-time used 95%% of a second)\n");
            vga_set_color_byte(color);
        }
    }
    return started;
}

// Rules 3 and 4: must 'current' give up the CPU on this tick?
static int turn_over(process_t *current) {
    if (rt_throttle_tick(current))
        return 1;                           // real-time time is up for this second
    if (!current || current->state != PROCESS_STATE_RUNNING)
        return 1;                           // it is sleeping or finished

    // Rule 3: the turn is over (SCHED_FIFO turns never end by time).
    if (policy_slice_ticks(current) != 0) {
        if (current->slice_left > 0)
            current->slice_left--;
        if (current->slice_left == 0)
            return 1;
    }

    // Rule 4: someone better is waiting. A SCHED_BATCH process never
    // pushes another off the CPU; it waits for the turn to end.
    int rescued;
    process_t *next = choose(current, &rescued);
    return next && next->policy != SCHED_BATCH &&
           priority_rank(next) > priority_rank(current);
}

int priority_tick(process_t *current) {
    int switch_now = turn_over(current);

    // Who runs after this tick? Everyone else who could run but does
    // not, waited one tick longer (aging, and pidstat's %wait). The one
    // about to get the CPU did not wait: it may have woken just now.
    int rescued;
    process_t *next = switch_now ? choose(current, &rescued) : current;
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p != current && p != next && p->state == PROCESS_STATE_READY) {
            p->waited_ticks++;
            p->ready_ticks++;
        }
    }
    return switch_now;
}

void priority_start_turn(process_t *p) {
    p->slice_left   = policy_slice_ticks(p);
    p->waited_ticks = 0;                    // aging starts over
}

int priority_set_nice(const process_t *caller, uint32_t pid, int nice) {
    process_t *target = process_get_by_pid(pid);

    // DEFENCE priority_bounds: a real nice value, a process you own, and
    // only root (or RLIMIT_NICE) may lower it.
    if (!def_nice_change_allowed(caller, target, nice))
        return 0;

    target->nice     = nice;
    target->priority = priority_level_of_nice(nice);
    return 1;
}

int priority_rt_throttled(void) {
    return rt_throttled;
}

uint32_t priority_rt_throttle_count(void) {
    return rt_throttle_count;
}
