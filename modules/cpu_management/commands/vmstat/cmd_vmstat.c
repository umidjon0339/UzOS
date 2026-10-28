/*
 * cmd_vmstat.c - `vmstat [interval [count]]`: the whole system in one line.
 *
 *   procs -----------memory---------- ---swap-- ---io---- -system-- ------cpu-----
 *    r  b   swpd   free   buff  cache   si   so   bi   bo   in   cs us sy id wa st
 *    1  0      0 120712      0      0    0    0    0    0  101   24  0  2 98  0  0
 *
 *   r      processes that want the CPU (READY or RUNNING)
 *   b      processes blocked on I/O (UzOS: none, nothing waits for a disk)
 *   free   free memory in KiB
 *   in     interrupts per second       cs   context switches per second
 *   us sy id   user, system, idle % of the CPU
 *
 * The first line is the average since boot (as in Linux); with an
 * interval, every next line covers that interval. Ctrl+C stops it.
 *
 * Uses: cpu_account_totals / _switches / _interrupts (core/accounting),
 *       loadavg_runnable (core/loadavg), pmm_get_free_blocks
 *       (memory_management), timer_uptime_ms (device_management),
 *       sys_sleep_interruptible (process_management), ui_* (ui),
 *       arg_next / atoi (lib).
 */

#include "modules/cpu_management/commands/vmstat/cmd_vmstat.h"      // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_account_*
#include "modules/cpu_management/core/loadavg/loadavg.h"            // loadavg_runnable
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_free_blocks
#include "modules/device_management/timer/timer.h"                  // timer_uptime_ms
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi

#define KB_PER_PAGE (PAGE_SIZE / 1024)

typedef struct {
    cpu_ticks_t ticks;
    uint32_t interrupts, switches, ms;
} sample_t;

static void take(sample_t *s) {
    cpu_account_totals(&s->ticks);
    s->interrupts = cpu_account_interrupts();
    s->switches   = cpu_account_switches();
    s->ms         = timer_uptime_ms();
}

static uint32_t pct(uint32_t part, uint32_t whole) {
    return whole ? part * 100 / whole : 0;
}

// One line for the time between samples a and b.
static void report(const sample_t *a, const sample_t *b) {
    uint32_t user = b->ticks.user + b->ticks.nice - a->ticks.user - a->ticks.nice;
    uint32_t sys  = b->ticks.system - a->ticks.system;
    uint32_t idle = b->ticks.idle - a->ticks.idle;
    uint32_t all  = user + sys + idle;
    uint32_t seconds = (b->ms - a->ms) / 1000;
    if (!seconds)
        seconds = 1;

    ui_printf("%2u %2u %6u %6u %6u %6u %4u %4u %4u %4u %4u %4u %2u %2u %2u %2u %2u\n",
              loadavg_runnable(), 0, 0, pmm_get_free_blocks() * KB_PER_PAGE, 0, 0, 0, 0, 0, 0,
              (b->interrupts - a->interrupts) / seconds, (b->switches - a->switches) / seconds,
              pct(user, all), pct(sys, all), pct(idle, all), 0, 0);
}

void cmd_vmstat(const char *args) {
    char word[16];
    int numbers[2] = { 0, 0 }, n = 0;
    for (args = arg_next(args, word, sizeof(word)); word[0] && n < 2;
         args = arg_next(args, word, sizeof(word)))
        numbers[n++] = atoi(word);
    int interval = numbers[0], count = n == 2 ? numbers[1] : (interval ? -1 : 1);

    ui_color(UI_TITLE);
    vga_print_str("procs -----------memory---------- ---swap-- ---io---- -system-- ------cpu-----\n");
    vga_print_str(" r  b   swpd   free   buff  cache   si   so   bi   bo   in   cs us sy id wa st\n");
    ui_reset();

    sample_t boot = { { 0, 0, 0, 0 }, 0, 0, 0 }, before, after;
    take(&before);
    report(&boot, &before);             // line 1: since boot

    for (int done = 1; count < 0 || done < count; done++) {
        if (!sys_sleep_interruptible(interval * 1000))
            break;                      // Ctrl+C
        take(&after);
        report(&before, &after);
        before = after;
    }
}
