/*
 * cmd_pidstat.c - `pidstat`: CPU use per process, per interval.
 *
 *   Time        UID    PID   %usr %system  %wait   %CPU  CPU  Command
 *   19:19:04      0      2   0.00    3.00   0.00   3.00    0  kernel_main
 *
 *   pidstat              since each process started
 *   pidstat 1 5          every second, five times, then the average
 *   pidstat -p 7 1       only PID 7
 *   pidstat -w 1         context switches instead:
 *                          cswch/s    voluntary (it slept, waited)
 *                          nvcswch/s  involuntary (preempted)
 *
 * %wait is the time a process was READY but another one had the CPU:
 * it wanted to run and could not. High %wait = the CPU is too busy.
 *
 * With an interval only processes that used the CPU in it are shown
 * (like Linux). Ctrl+C stops it.
 *
 * Uses: the process table (process_management), affinity_current_cpu
 *       (core/affinity), timer_* / rtc_read (device_management),
 *       sys_sleep_interruptible, def_safe_text (ui defence safe_text),
 *       ui_* (ui), arg_next / atoi / strcmp (lib).
 */

#include "modules/cpu_management/commands/pidstat/cmd_pidstat.h"    // this command
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_current_cpu
#include "modules/process_management/core/process/process.h"        // process table
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/device_management/timer/timer.h"                  // timer_ticks
#include "modules/device_management/rtc/rtc.h"                      // rtc_read
#include "modules/ui/security/defence/safe_text/def_safe_text.h"    // def_safe_text
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp

#define NAME_WIDTH 20

// The counters of one process at one moment.
typedef struct {
    uint32_t pid;               // 0 = empty
    uint32_t user, system, ready, nvcsw, nivcsw;
} snap_t;

typedef struct {
    int switches;               // -w
    uint32_t only_pid;          // -p
} pidstat_options_t;

static void take(snap_t *s) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        s[i].pid = p->state == PROCESS_STATE_FREE ? 0 : p->pid;
        s[i].user   = p->user_ticks;
        s[i].system = p->kernel_ticks;
        s[i].ready  = p->ready_ticks;
        s[i].nvcsw  = p->nvcsw;
        s[i].nivcsw = p->nivcsw;
    }
}

static void timestamp(const char *label) {
    rtc_time_t now;
    if (label)
        ui_printf("%-8s", label);
    else if (rtc_read(&now))
        ui_printf("%02d:%02d:%02d", now.hour, now.minute, now.second);
    else
        vga_print_str("--:--:--");
}

// "  3.00": ticks out of 'whole' ticks, as a percentage.
static void pct(uint32_t ticks, uint32_t whole) {
    uint32_t h = whole ? (uint32_t)((uint64_t)ticks * 10000 / whole) : 0;
    ui_printf(" %3u.%02u", h / 100, h % 100);
}

static void header(const pidstat_options_t *o) {
    ui_color(UI_TITLE);
    if (o->switches)
        vga_print_str("Time        UID    PID   cswch/s nvcswch/s  Command\n");
    else
        vga_print_str("Time        UID    PID   %usr %system  %wait   %CPU  CPU  Command\n");
    ui_reset();
}

// One report from 'a' to 'b' over 'ticks' timer ticks. a = NULL: since start.
static void report(const pidstat_options_t *o, const snap_t *a, const snap_t *b,
                   uint32_t ticks, const char *label) {
    process_t *table = process_get_table();
    uint32_t hz = timer_frequency() ? timer_frequency() : 100;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!b[i].pid || (o->only_pid && b[i].pid != o->only_pid))
            continue;
        snap_t d = b[i];
        uint32_t span = ticks;
        if (a) {
            if (a[i].pid != b[i].pid)
                continue;                       // a different process in that slot now
            d.user -= a[i].user;
            d.system -= a[i].system;
            d.ready -= a[i].ready;
            d.nvcsw -= a[i].nvcsw;
            d.nivcsw -= a[i].nivcsw;
            if (!o->switches && !d.user && !d.system && !o->only_pid)
                continue;                       // did not run: skip, as Linux does
        } else {
            span = timer_ticks - table[i].start_tick;   // since it started
        }
        if (!span)
            span = 1;

        char name[NAME_WIDTH + 1];
        def_safe_text(table[i].name, name, NAME_WIDTH);
        timestamp(label);
        ui_printf(" %6u %6u", table[i].uid, b[i].pid);
        if (o->switches) {
            uint32_t seconds100 = span * 100 / hz;      // hundredths of a second
            uint32_t v = seconds100 ? d.nvcsw * 10000 / seconds100 : 0;
            uint32_t n = seconds100 ? d.nivcsw * 10000 / seconds100 : 0;
            ui_printf(" %6u.%02u %6u.%02u  %s\n", v / 100, v % 100, n / 100, n % 100, name);
        } else {
            pct(d.user, span);
            ui_printf(" ");
            pct(d.system, span);
            pct(d.ready, span);
            pct(d.user + d.system, span);
            ui_printf("  %3d  %s\n", affinity_current_cpu(), name);
        }
    }
}

void cmd_pidstat(const char *args) {
    pidstat_options_t o = { 0, 0 };
    char word[16];
    int numbers[2] = { 0, 0 }, n = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-w") == 0) {
            o.switches = 1;
        } else if (strcmp(word, "-u") == 0) {
            o.switches = 0;
        } else if (strcmp(word, "-p") == 0) {
            args = arg_next(args, word, sizeof(word));
            o.only_pid = atoi(word);
        } else if (word[0] >= '0' && word[0] <= '9' && n < 2) {
            numbers[n++] = atoi(word);
        } else {
            ui_usage("pidstat [-w] [-p PID] [interval [count]]");
            return;
        }
    }
    int interval = numbers[0], count = n == 2 ? numbers[1] : -1;

    static snap_t first[MAX_PROCESSES], before[MAX_PROCESSES], after[MAX_PROCESSES];
    header(&o);
    take(first);
    if (!interval) {
        report(&o, 0, first, 0, 0);
        return;
    }

    uint32_t start = timer_ticks, last = start;
    int done = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        before[i] = first[i];
    while (count < 0 || done < count) {
        if (!sys_sleep_interruptible(interval * 1000))
            break;                              // Ctrl+C
        take(after);
        report(&o, before, after, timer_ticks - last, 0);
        last = timer_ticks;
        for (int i = 0; i < MAX_PROCESSES; i++)
            before[i] = after[i];
        done++;
        if (count < 0 || done < count)
            vga_print_str("\n");
    }
    if (done) {
        vga_print_str("\n");
        report(&o, first, before, last - start, "Average:");
    }
}
