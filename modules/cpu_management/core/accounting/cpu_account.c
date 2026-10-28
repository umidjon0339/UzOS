/*
 * cpu_account.c - Charge every timer tick to a process; time every
 *                 task switch with the cycle counter.
 *
 * This runs inside the timer interrupt, so it must be short: a few
 * additions per tick, and one walk over the process table per second.
 * On the same tick it gives three other parts their turn:
 *   rlimit_check_cpu  (core/rlimit)   RLIMIT_CPU used up? end the process
 *   perf_sample       (core/perf)     `perf record` takes a sample
 *   loadavg_tick      (core/loadavg)  every 5 s: the load average
 *
 * Uses: scheduler_get_current_process / scheduler_idle_process
 *       (process_management), process_get_table, timer_frequency /
 *       timer_ticks (device_management), isr_irq_count (arch/x86/isr),
 *       tsc_now (core/tsc).
 */

#include "modules/cpu_management/core/accounting/cpu_account.h"   // this part
#include "modules/cpu_management/core/tsc/tsc.h"                  // tsc_now
#include "modules/cpu_management/core/rlimit/rlimit.h"            // rlimit_check_cpu
#include "modules/cpu_management/core/perf/perf.h"                // perf_sample
#include "modules/cpu_management/core/loadavg/loadavg.h"          // loadavg_tick, loadavg_runnable
#include "modules/process_management/core/scheduler/scheduler.h"  // current / idle process
#include "modules/device_management/timer/timer.h"                // timer_frequency, timer_ticks

static cpu_load_t load;                 // the last full second

// The second being counted now.
static uint32_t window_length;          // ticks so far
static uint32_t window_user, window_nice, window_kernel, window_idle;
static uint32_t window_switches, window_irqs_at_start;

// Since boot.
static cpu_ticks_t totals;
static uint32_t switches;

// The last CPU_HISTORY_SECONDS seconds, a ring: 'history_next' is the
// slot the next second goes into.
static cpu_second_t history[CPU_HISTORY_SECONDS];
static int history_next, history_used;

static uint32_t percent(uint32_t part, uint32_t whole) {
    return whole ? part * 100 / whole : 0;
}

uint32_t cpu_account_interrupts(void) {
    uint32_t n = 0;
    for (int irq = 0; irq < ISR_IRQS; irq++)
        n += isr_irq_count(irq);
    return n;
}

// A second is over: turn the counts into percentages, start again.
static void close_window(void) {
    load.user   = percent(window_user, window_length);
    load.nice   = percent(window_nice, window_length);
    load.kernel = percent(window_kernel, window_length);
    load.idle   = percent(window_idle, window_length);
    load.busy   = load.user + load.nice + load.kernel;

    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        table[i].cpu_percent  = percent(table[i].window_ticks, window_length);
        table[i].window_ticks = 0;
    }

    uint32_t irqs = cpu_account_interrupts();
    cpu_second_t *s = &history[history_next];
    s->uptime_s   = timer_ticks / (timer_frequency() ? timer_frequency() : 1);
    s->load       = load;
    s->interrupts = irqs - window_irqs_at_start;
    s->switches   = window_switches;
    s->runnable   = loadavg_runnable();
    history_next = (history_next + 1) % CPU_HISTORY_SECONDS;
    if (history_used < CPU_HISTORY_SECONDS)
        history_used++;

    window_length = window_user = window_nice = window_kernel = window_idle = 0;
    window_switches = 0;
    window_irqs_at_start = irqs;
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
        totals.idle++;
    } else if (ring3) {
        p->user_ticks++;
        if (p->nice > 0) {              // a "nice" program: counted apart
            window_nice++;
            totals.nice++;
        } else {
            window_user++;
            totals.user++;
        }
    } else {
        p->kernel_ticks++;
        window_kernel++;
        totals.system++;
    }

    rlimit_check_cpu(p);                // RLIMIT_CPU: over the limit ends it
    perf_sample(regs, p);               // `perf record` running? take a sample
    loadavg_tick();

    if (++window_length >= timer_frequency())
        close_window();
}

void cpu_account_switch(process_t *prev, process_t *next) {
    uint64_t now = tsc_now();

    switches++;
    window_switches++;
    if (prev) {
        prev->cycles += now - prev->cycles_since;
        // Still RUNNING = the scheduler took the CPU away (preempted).
        // Anything else = it slept, waited or ended: it gave it up.
        if (prev->state == PROCESS_STATE_RUNNING)
            prev->nivcsw++;
        else
            prev->nvcsw++;
    }
    if (next)
        next->cycles_since = now;
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

uint64_t cpu_account_cycles(const process_t *p) {
    if (!p)
        return 0;
    if (p == scheduler_get_current_process())
        return p->cycles + (tsc_now() - p->cycles_since);   // still running
    return p->cycles;
}

uint32_t cpu_busy_ms(void) {
    return ticks_to_ms(totals.user + totals.nice + totals.system);
}

uint32_t cpu_idle_ms(void) {
    return ticks_to_ms(totals.idle);
}

void cpu_account_totals(cpu_ticks_t *out) {
    *out = totals;
}

uint32_t cpu_account_switches(void) {
    return switches;
}

int cpu_history_count(void) {
    return history_used;
}

const cpu_second_t *cpu_history(int index) {
    if (index < 0 || index >= history_used)
        return 0;
    int slot = (history_next - 1 - index + CPU_HISTORY_SECONDS) % CPU_HISTORY_SECONDS;
    return &history[slot];
}
