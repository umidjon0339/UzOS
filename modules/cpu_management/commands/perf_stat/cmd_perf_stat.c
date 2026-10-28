/*
 * cmd_perf_stat.c - `perf stat command`: run it, then count what it did.
 *
 *    Performance counter stats for 'ls':
 *
 *              12.34 msec task-clock         #    0.410 CPUs utilized
 *                  3      context-switches   #  243.112 /sec
 *                  0      cpu-migrations     #    0.000 /sec
 *                  0      page-faults        #    0.000 /sec
 *         33,145,021      cycles             #    2.686 GHz
 *    <not supported>      instructions
 *
 *        0.030102345 seconds time elapsed
 *
 * task-clock and cycles are EXACT: the cycle counter is read on every
 * task switch (core/accounting), so they count only the time the
 * command really had the CPU. "CPUs utilized" = task-clock / elapsed:
 * below 1 means it also waited. The command runs as its own process
 * (run_child.c, this folder); processes it starts are counted too.
 * instructions needs a hardware PMU, which UzOS does not drive.
 *
 * Uses: perf_stat_run_child (run_child.c, this folder), tsc_mhz / tsc_cycles_to_us (core/tsc),
 *       timer_frequency (device_management), ui_* (ui).
 */

#include "modules/cpu_management/commands/perf_stat/cmd_perf_stat.h"    // this command
#include "modules/cpu_management/commands/perf_stat/run_child.h"        // perf_stat_run_child (this folder)
#include "modules/cpu_management/core/tsc/tsc.h"                        // tsc_*
#include "modules/device_management/timer/timer.h"                      // timer_frequency
#include "modules/ui/core/style/ui.h"                                   // ui_*

// A 64-bit number with thousands separators, right-aligned in 'width'.
static void print_count(uint64_t n, int width) {
    char digits[32];
    int len = 0, group = 0;
    do {
        if (group == 3) {
            digits[len++] = ',';
            group = 0;
        }
        digits[len++] = '0' + (char)(n % 10);
        n /= 10;
        group++;
    } while (n);
    for (int i = len; i < width; i++)
        vga_print_str(" ");
    while (len)
        vga_put_char(digits[--len]);
}

// "0.030102345": microseconds as seconds with nine places.
static void print_seconds(uint32_t us) {
    ui_printf("%u.%06u000", us / 1000000, us % 1000000);
}

// "243.112 /sec": count per second of 'us'.
static void per_second(uint32_t count, uint32_t us) {
    uint32_t milli = us ? (uint32_t)((uint64_t)count * 1000000000ull / us) : 0;   // x 1000
    ui_printf("#  %5u.%03u /sec", milli / 1000, milli % 1000);
}

static void row(uint32_t count, const char *name, uint32_t us) {
    print_count(count, 18);
    ui_printf("      %-18s ", name);
    ui_color(UI_DIM);
    per_second(count, us);
    ui_reset();
    vga_print_str("\n");
}

void cmd_perf_stat(const char *args) {
    if (!args[0]) {
        ui_usage("perf stat command");
        return;
    }

    perf_stat_result_t r;
    if (perf_stat_run_child(args, &r) < 0) {
        ui_error("perf", "cannot run that command");
        return;
    }

    uint32_t task_us = tsc_cycles_to_us(r.usage.cycles);
    uint32_t real_us = tsc_mhz() ? tsc_cycles_to_us(r.real_cycles) : r.real_ms * 1000;
    uint32_t hz = timer_frequency() ? timer_frequency() : 100;
    uint32_t switches = r.usage.nvcsw + r.usage.nivcsw;
    uint32_t used_milli = real_us ? (uint32_t)((uint64_t)task_us * 1000 / real_us) : 0;

    ui_printf("\n Performance counter stats for '%s':\n\n", args);

    ui_printf("%14u.%02u msec task-clock         ", task_us / 1000, task_us / 10 % 100);
    ui_color(UI_DIM);
    ui_printf("#  %5u.%03u CPUs utilized\n", used_milli / 1000, used_milli % 1000);
    ui_reset();
    row(switches, "context-switches", real_us);
    row(0, "cpu-migrations", real_us);
    row(0, "page-faults", real_us);

    print_count(r.usage.cycles, 18);
    ui_printf("      %-18s ", "cycles");
    ui_color(UI_DIM);
    uint32_t mhz_used = task_us ? (uint32_t)(r.usage.cycles / task_us) : 0;
    ui_printf("#  %5u.%03u GHz\n", mhz_used / 1000, mhz_used % 1000);
    ui_reset();
    ui_printf("%18s      %s\n\n", "<not supported>", "instructions");

    vga_print_str("   ");
    print_seconds(real_us);
    vga_print_str(" seconds time elapsed\n\n   ");
    print_seconds(r.usage.user_ticks * (1000000 / hz));
    vga_print_str(" seconds user\n   ");
    print_seconds(r.usage.kernel_ticks * (1000000 / hz));
    vga_print_str(" seconds sys\n");
}
