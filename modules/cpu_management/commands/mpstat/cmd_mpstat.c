/*
 * cmd_mpstat.c - `mpstat [-P ALL|0] [interval [count]]`: CPU use per CPU.
 *
 *   UzOS (uzos)   10/26/26   _i686_   (1 CPU)
 *
 *   19:19:03  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal   %idle
 *   19:19:04  all    0.00    0.00    3.00    0.00    0.00    0.00    0.00   97.00
 *   Average:  all    0.00    0.00    3.00    0.00    0.00    0.00    0.00   97.00
 *
 *   mpstat              one line: the averages since boot
 *   mpstat 1 5          every second, five times, then the average
 *   mpstat -P ALL 1     each CPU too (UzOS: "all" and CPU 0)
 *
 * How: read the tick counters (the same as /proc/stat), wait, read
 * again; the differences are the CPU use of that interval. %iowait,
 * %irq, %soft and %steal stay 0: UzOS has no disks to wait for, does
 * not count interrupt time apart, and is not a hypervisor guest that
 * could be stolen from... as far as it can tell.
 * Ctrl+C stops it.
 *
 * Uses: cpu_account_totals (core/accounting), affinity_cpu_count
 *       (core/affinity), sys_sleep_interruptible (process_management),
 *       rtc_read (device_management), ui_* (ui), arg_next / atoi /
 *       strcmp (lib).
 */

#include "modules/cpu_management/commands/mpstat/cmd_mpstat.h"      // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_account_totals
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_cpu_count
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/device_management/rtc/rtc.h"                      // rtc_read
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp

// "12.34": part / whole in percent with two decimals.
static void percent(uint32_t part, uint32_t whole) {
    uint32_t hundredths = whole ? (uint32_t)((uint64_t)part * 10000 / whole) : 0;
    ui_printf(" %4u.%02u", hundredths / 100, hundredths % 100);
}

static void timestamp(void) {
    rtc_time_t now;
    if (rtc_read(&now))
        ui_printf("%02d:%02d:%02d", now.hour, now.minute, now.second);
    else
        vga_print_str("--:--:--");
}

static void row(const char *cpu, const cpu_ticks_t *d) {
    uint32_t total = d->user + d->nice + d->system + d->idle;
    ui_printf(" %4s", cpu);
    percent(d->user, total);
    percent(d->nice, total);
    percent(d->system, total);
    percent(0, total);                  // %iowait
    percent(0, total);                  // %irq
    percent(0, total);                  // %soft
    percent(0, total);                  // %steal
    percent(d->idle, total);
    vga_print_str("\n");
}

static void rows(const char *when, const cpu_ticks_t *d, int all_cpus) {
    if (when) vga_print_str(when); else timestamp();
    row("all", d);
    if (all_cpus) {
        for (int n = 0; n < affinity_cpu_count(); n++) {
            if (when) vga_print_str(when); else timestamp();
            row("0", d);                // one CPU: the same numbers
        }
    }
}

static void diff(const cpu_ticks_t *a, const cpu_ticks_t *b, cpu_ticks_t *out) {
    out->user   = b->user - a->user;
    out->nice   = b->nice - a->nice;
    out->system = b->system - a->system;
    out->idle   = b->idle - a->idle;
}

void cmd_mpstat(const char *args) {
    char word[16];
    int all_cpus = 0, numbers[2] = { 0, 0 }, n = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-P") == 0) {
            args = arg_next(args, word, sizeof(word));
            all_cpus = strcmp(word, "ALL") == 0 || strcmp(word, "0") == 0;
        } else if (word[0] >= '0' && word[0] <= '9' && n < 2) {
            numbers[n++] = atoi(word);
        } else {
            ui_usage("mpstat [-P ALL|0] [interval [count]]");
            return;
        }
    }
    int interval = numbers[0], count = n == 2 ? numbers[1] : (interval ? -1 : 1);

    ui_printf("UzOS (uzos)   _i686_   (%d CPU)\n\n", affinity_cpu_count());
    ui_color(UI_TITLE);
    vga_print_str("Time      CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal   %idle\n");
    ui_reset();

    cpu_ticks_t first, before, after, d;
    cpu_account_totals(&first);
    if (!interval) {                    // no interval: since boot
        rows(0, &first, all_cpus);
        return;
    }

    before = first;
    int done = 0;
    while (count < 0 || done < count) {
        if (!sys_sleep_interruptible(interval * 1000))
            break;                      // Ctrl+C
        cpu_account_totals(&after);
        diff(&before, &after, &d);
        rows(0, &d, all_cpus);
        before = after;
        done++;
    }
    if (done) {
        diff(&first, &before, &d);
        rows("Average:", &d, all_cpus);
    }
}
