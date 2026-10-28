/*
 * loadavg.c - Sample the runnable count every 5 s, keep three averages.
 *
 * The constants are Linux's (kernel/sched/loadavg.c): e^(-5/60),
 * e^(-5/300), e^(-5/900) in the same fixed point.
 *
 * Uses: the process table and the idle task (process_management),
 *       timer_frequency (device_management), format (lib/format).
 */

#include "modules/cpu_management/core/loadavg/loadavg.h"         // this part
#include "modules/process_management/core/scheduler/scheduler.h" // scheduler_idle_process
#include "modules/device_management/timer/timer.h"               // timer_frequency
#include "lib/format/format.h"                                   // format

#define EXP_1  1884     // 1 minute
#define EXP_5  2014     // 5 minutes
#define EXP_15 2037     // 15 minutes

static uint32_t averages[3];    // fixed point, LOADAVG_ONE = 1.00
static uint32_t ticks;

uint32_t loadavg_runnable(void) {
    process_t *table = process_get_table();
    uint32_t n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p == scheduler_idle_process())
            continue;
        if (p->state == PROCESS_STATE_RUNNING || p->state == PROCESS_STATE_READY)
            n++;
    }
    return n;
}

static uint32_t blend(uint32_t load, uint32_t e, uint32_t active) {
    return (load * e + active * (LOADAVG_ONE - e)) >> LOADAVG_SHIFT;
}

void loadavg_tick(void) {
    uint32_t hz = timer_frequency();
    if (!hz || ++ticks < LOADAVG_SAMPLE_SECONDS * hz)
        return;
    ticks = 0;

    uint32_t active = loadavg_runnable() * LOADAVG_ONE;
    averages[0] = blend(averages[0], EXP_1, active);
    averages[1] = blend(averages[1], EXP_5, active);
    averages[2] = blend(averages[2], EXP_15, active);
}

void loadavg_get(uint32_t hundredths[3]) {
    for (int i = 0; i < 3; i++)
        hundredths[i] = (averages[i] * 100 + LOADAVG_ONE / 2) >> LOADAVG_SHIFT;
}

void loadavg_format(uint32_t hundredths, char *out, int size) {
    format(out, size, "%u.%02u", hundredths / 100, hundredths % 100);
}
