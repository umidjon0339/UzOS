/*
 * cpu_account.c - Charge every timer tick to a process.
 *
 * This runs inside the timer interrupt, so it must be short: a few
 * additions per tick, and one walk over the process table per second.
 *
 * Uses: scheduler_get_current_process / scheduler_idle_process
 *       (process_management), process_get_table, timer_frequency
 *       (device_management/timer).
 */

#include "cpu_account.h"
#include "scheduler.h"
#include "timer.h"

static cpu_load_t load;                 // the last full second

// The second being counted now.
static uint32_t window_length;          // ticks so far
static uint32_t window_user, window_kernel, window_idle;

// Since boot.
static uint32_t busy_ticks, idle_ticks;

static uint32_t percent(uint32_t part, uint32_t whole) {
    return whole ? part * 100 / whole : 0;
}

// A second is over: turn the counts into percentages, start again.
static void close_window(void) {
    load.user   = percent(window_user, window_length);
    load.kernel = percent(window_kernel, window_length);
    load.idle   = percent(window_idle, window_length);
    load.busy   = load.user + load.kernel;

    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        table[i].cpu_percent  = percent(table[i].window_ticks, window_length);
        table[i].window_ticks = 0;
    }
    window_length = window_user = window_kernel = window_idle = 0;
}

void cpu_account_tick(const registers_t *regs) {
    process_t *p = scheduler_get_current_process();
    if (!p)
        return;                         // before the scheduler exists

    int ring3 = (regs->cs & 3) == 3;    // the saved CS says who was interrupted
    p->window_ticks++;

    if (p == scheduler_idle_process()) {
        p->kernel_ticks++;
        window_idle++;
        idle_ticks++;
    } else if (ring3) {
        p->user_ticks++;
        window_user++;
        busy_ticks++;
    } else {
        p->kernel_ticks++;
        window_kernel++;
        busy_ticks++;
    }

    if (++window_length >= timer_frequency())
        close_window();
}

const cpu_load_t *cpu_load(void) {
    return &load;
}

static uint32_t ticks_to_ms(uint32_t ticks) {
    uint32_t hz = timer_frequency();
    return hz ? (uint32_t)((uint64_t)ticks * 1000 / hz) : 0;
}

uint32_t cpu_account_ms(const process_t *p) {
    return p ? ticks_to_ms(p->user_ticks + p->kernel_ticks) : 0;
}

uint32_t cpu_busy_ms(void) {
    return ticks_to_ms(busy_ticks);
}

uint32_t cpu_idle_ms(void) {
    return ticks_to_ms(idle_ticks);
}
