/*
 * cmd_time.c - `time command`: how long it took, and whose time it was.
 *
 *   $ time ls
 *   ...output of ls...
 *
 *   real    0m0.031s       wall clock: from start to end
 *   user    0m0.000s       CPU time in ring 3 (the program itself)
 *   sys     0m0.030s       CPU time in ring 0 (the kernel, for it)
 *
 * real > user + sys: the command waited (slept, or other processes ran).
 * real < user + sys can only happen with several CPUs.
 *
 * Shell commands are kernel code, so their time is "sys"; a ring 3
 * program (exec, seclab ... live) shows "user" time too. The command
 * runs as its own process (run_child.c, this folder); its usage record, including
 * any processes it started, comes from core/task.
 *
 * user and sys are counted in timer ticks (10 ms each); real is
 * measured with the TSC, to the microsecond.
 *
 * Uses: time_run_child (run_child.c, this folder), tsc_cycles_to_us (core/tsc),
 *       timer_frequency (device_management), ui_* (ui).
 */

#include "modules/cpu_management/commands/time/cmd_time.h"      // this command
#include "modules/cpu_management/commands/time/run_child.h"             // time_run_child (this folder)
#include "modules/cpu_management/core/tsc/tsc.h"                // tsc_cycles_to_us
#include "modules/device_management/timer/timer.h"              // timer_frequency
#include "modules/ui/core/style/ui.h"                           // ui_*

// "0m0.031s" from microseconds.
static void print_time(const char *label, uint32_t us) {
    uint32_t ms = us / 1000;
    ui_printf("%s\t%um%u.%03us\n", label, ms / 60000, ms / 1000 % 60, ms % 1000);
}

void cmd_time(const char *args) {
    if (!args[0]) {
        ui_usage("time command");
        return;
    }

    time_result_t r;
    if (time_run_child(args, &r) < 0) {
        ui_error("time", "cannot run that command");
        return;
    }

    uint32_t us_per_tick = 1000000 / (timer_frequency() ? timer_frequency() : 100);
    uint32_t real_us = tsc_mhz() ? tsc_cycles_to_us(r.real_cycles) : r.real_ms * 1000;

    vga_print_str("\n");
    print_time("real", real_us);
    print_time("user", r.usage.user_ticks * us_per_tick);
    print_time("sys", r.usage.kernel_ticks * us_per_tick);
}
