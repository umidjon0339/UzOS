/*
 * cmd_sar.c - `sar`: System Activity Reporter.
 *
 *   sar               the last minute, from the kernel's own history
 *                     (one line per 10 seconds; Linux's sar reads a log
 *                     its collector wrote every 10 minutes)
 *   sar 1 5           live: every second, five times, then the average
 *   sar -u ...        CPU use (the default)
 *   sar -q ...        run queue: runq-sz, plist-sz, load averages
 *   sar -w ...        task creation and context switches per second
 *
 *   Time        CPU     %user     %nice   %system   %iowait    %steal     %idle
 *   19:19:04    all      0.00      0.00      3.00      0.00      0.00     97.00
 *
 * Uses: cpu_history / cpu_account_* (core/accounting), loadavg_*
 *       (core/loadavg), cpu_task_forks (core/task), the process table
 *       (process_management), timer_* / rtc_read (device_management),
 *       sys_sleep_interruptible, ui_* (ui), arg_next / atoi / strcmp (lib).
 */

#include "modules/cpu_management/commands/sar/cmd_sar.h"            // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_history, cpu_account_*
#include "modules/cpu_management/core/loadavg/loadavg.h"            // loadavg_*
#include "modules/cpu_management/core/task/cpu_task.h"              // cpu_task_forks
#include "modules/process_management/core/process/process.h"        // the process table
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/device_management/timer/timer.h"                  // timer_uptime_ms
#include "modules/device_management/rtc/rtc.h"                      // rtc_read
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp

#define HISTORY_STEP 10         // seconds per line of `sar` without an interval

typedef enum { REPORT_CPU, REPORT_QUEUE, REPORT_TASKS } report_t;

// What a line shows, summed over its seconds.
typedef struct {
    uint32_t user, nice, system, idle;      // ticks
    uint32_t switches, forks, seconds;
} totals_t;

// "19:19:03", the clock 'ago' seconds before now.
static void clock_ago(uint32_t ago) {
    rtc_time_t now;
    if (!rtc_read(&now)) {
        vga_print_str("--:--:--");
        return;
    }
    int32_t s = now.hour * 3600 + now.minute * 60 + now.second - (int32_t)ago;
    while (s < 0)
        s += 24 * 3600;
    ui_printf("%02d:%02d:%02d", s / 3600, s / 60 % 60, s % 60);
}

static void pct(uint32_t part, uint32_t whole) {
    uint32_t h = whole ? (uint32_t)((uint64_t)part * 10000 / whole) : 0;
    ui_printf(" %6u.%02u", h / 100, h % 100);
}

static void header(report_t r) {
    ui_color(UI_TITLE);
    if (r == REPORT_CPU)
        vga_print_str("Time        CPU     %user     %nice   %system   %iowait    %steal     %idle\n");
    else if (r == REPORT_QUEUE)
        vga_print_str("Time      runq-sz  plist-sz   ldavg-1   ldavg-5  ldavg-15   blocked\n");
    else
        vga_print_str("Time         proc/s   cswch/s\n");
    ui_reset();
}

static int process_count(void) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        n += t[i].state != PROCESS_STATE_FREE;
    return n;
}

// One line. label = NULL: a time 'ago' seconds back.
static void line(report_t r, const char *label, uint32_t ago, const totals_t *t) {
    if (label) ui_printf("%-8s", label); else clock_ago(ago);
    if (r == REPORT_CPU) {
        uint32_t all = t->user + t->nice + t->system + t->idle;
        vga_print_str("    all ");
        pct(t->user, all);
        pct(t->nice, all);
        pct(t->system, all);
        pct(0, all);
        pct(0, all);
        pct(t->idle, all);
        vga_print_str("\n");
    } else if (r == REPORT_QUEUE) {
        uint32_t load[3];
        char a[8], b[8], c[8];
        loadavg_get(load);
        loadavg_format(load[0], a, sizeof(a));
        loadavg_format(load[1], b, sizeof(b));
        loadavg_format(load[2], c, sizeof(c));
        ui_printf(" %8u %9d %9s %9s %9s %9u\n", loadavg_runnable(), process_count(), a, b, c, 0);
    } else {
        uint32_t s = t->seconds ? t->seconds : 1;
        ui_printf(" %7u.%02u %7u.%02u\n", t->forks / s, t->forks * 100 / s % 100,
                  t->switches / s, t->switches * 100 / s % 100);
    }
}

// sar with no interval: the kernel's last-minute history, 10 s per line.
static void history(report_t r) {
    int count = cpu_history_count();
    if (!count) {
        ui_info("sar", "no history yet: wait a second");
        return;
    }
    totals_t all = { 0 };
    for (int start = count - 1; start >= 0; start -= HISTORY_STEP) {
        totals_t t = { 0 };
        int end = start - HISTORY_STEP + 1 < 0 ? 0 : start - HISTORY_STEP + 1;
        for (int i = start; i >= end; i--) {    // older to newer
            const cpu_second_t *s = cpu_history(i);
            t.user += s->load.user;             // percent of each second = weight 1 each
            t.nice += s->load.nice;
            t.system += s->load.kernel;
            t.idle += s->load.idle;
            t.switches += s->switches;
            t.seconds++;
        }
        line(r, 0, end, &t);
        all.user += t.user; all.nice += t.nice; all.system += t.system; all.idle += t.idle;
        all.switches += t.switches; all.seconds += t.seconds;
    }
    line(r, "Average:", 0, &all);
}

static void sample(totals_t *t) {
    cpu_ticks_t ticks;
    cpu_account_totals(&ticks);
    t->user = ticks.user;
    t->nice = ticks.nice;
    t->system = ticks.system;
    t->idle = ticks.idle;
    t->switches = cpu_account_switches();
    t->forks = cpu_task_forks();
    t->seconds = timer_uptime_ms() / 1000;
}

static void difference(const totals_t *a, const totals_t *b, totals_t *d) {
    d->user = b->user - a->user;
    d->nice = b->nice - a->nice;
    d->system = b->system - a->system;
    d->idle = b->idle - a->idle;
    d->switches = b->switches - a->switches;
    d->forks = b->forks - a->forks;
    d->seconds = b->seconds - a->seconds;
}

void cmd_sar(const char *args) {
    report_t r = REPORT_CPU;
    char word[16];
    int numbers[2] = { 0, 0 }, n = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-u") == 0)      r = REPORT_CPU;
        else if (strcmp(word, "-q") == 0) r = REPORT_QUEUE;
        else if (strcmp(word, "-w") == 0) r = REPORT_TASKS;
        else if (word[0] >= '0' && word[0] <= '9' && n < 2) numbers[n++] = atoi(word);
        else {
            ui_usage("sar [-u|-q|-w] [interval [count]]");
            return;
        }
    }

    header(r);
    if (!numbers[0]) {
        history(r);
        return;
    }

    int interval = numbers[0], count = n == 2 ? numbers[1] : -1, done = 0;
    totals_t first, before, after, d;
    sample(&first);
    before = first;
    while (count < 0 || done < count) {
        if (!sys_sleep_interruptible(interval * 1000))
            break;                              // Ctrl+C
        sample(&after);
        difference(&before, &after, &d);
        line(r, 0, 0, &d);
        before = after;
        done++;
    }
    if (done) {
        difference(&first, &before, &d);
        line(r, "Average:", 0, &d);
    }
}
