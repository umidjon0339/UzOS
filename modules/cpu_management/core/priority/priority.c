/*
 * priority.c - Priority scheduling with time slices and aging.
 *
 * Runs inside the timer interrupt (priority_tick) and inside
 * scheduler_yield (priority_pick_next), so it only walks the process
 * table: 64 slots, a few comparisons each.
 *
 * Uses: process_get_table / process_get_by_pid (process_management),
 *       and two defences:
 *         aging           - waiting raises the effective priority
 *         priority_bounds - who may set which level
 */

#include "priority.h"
#include "def_aging.h"
#include "def_priority_bounds.h"

static int runnable(const process_t *p) {
    return p->state == PROCESS_STATE_READY || p->state == PROCESS_STATE_RUNNING;
}

int priority_effective(const process_t *p) {
    // DEFENCE aging: the longer a process waits, the higher it ranks.
    return def_aging_effective(p->priority, p->waited_ticks);
}

// Rules 1 and 2. *rescued = 1 if the winner only won thanks to aging:
// its own (base) priority is below another runnable process's.
static process_t *choose(process_t *current, int *rescued) {
    process_t *table = process_get_table();
    int start = current ? current - table : 0;
    process_t *best = 0;
    int best_level = -1, top_base = -1;

    // Start AFTER current and end with current itself, so among equal
    // priorities everyone else comes first: that is the round robin.
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        process_t *p = &table[(start + i) % MAX_PROCESSES];
        if (!runnable(p))
            continue;
        int level = priority_effective(p);
        if (level > best_level) {           // strictly better: ties keep the earlier one
            best = p;
            best_level = level;
        }
        if (p->priority > top_base)
            top_base = p->priority;
    }

    *rescued = best && best->priority < top_base;
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

int priority_tick(process_t *current) {
    process_t *table = process_get_table();

    // Everyone who could run but does not, waits one tick longer.
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (&table[i] != current && table[i].state == PROCESS_STATE_READY)
            table[i].waited_ticks++;

    if (!current || current->state != PROCESS_STATE_RUNNING)
        return 1;                           // it is sleeping or finished
    if (current->slice_left > 0)
        current->slice_left--;
    if (current->slice_left == 0)
        return 1;                           // rule 3: the turn is over

    // Rule 4: someone better is waiting.
    int rescued;
    process_t *next = choose(current, &rescued);
    return next && priority_effective(next) > priority_effective(current);
}

void priority_start_turn(process_t *p) {
    p->slice_left   = CPU_SLICE_TICKS;
    p->waited_ticks = 0;                    // aging starts over
}

int priority_set(const process_t *caller, uint32_t pid, int level) {
    process_t *target = process_get_by_pid(pid);

    // DEFENCE priority_bounds: a real level, a process you own, and only
    // root may raise one.
    if (!def_priority_change_allowed(caller, target, level))
        return 0;

    target->priority = level;
    return 1;
}
