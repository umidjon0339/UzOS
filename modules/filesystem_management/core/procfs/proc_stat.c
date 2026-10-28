/*
 * proc_stat.c - /proc/stat: everything the CPU did since boot, as counters.
 *
 *     cpu  12 0 340 52310 0 0 0 0 0 0      all CPUs, in ticks (1/100 s):
 *     cpu0 12 0 340 52310 0 0 0 0 0 0      user nice system idle iowait irq
 *                                          softirq steal guest guest_nice
 *     intr 53402 52662 512 0 ...           interrupts: total, then per IRQ line
 *     ctxt 8122                            context switches
 *     btime 1792987200                     boot time, seconds since 1970
 *     processes 31                         processes created since boot
 *     procs_running 1                      READY or RUNNING now
 *     procs_blocked 0                      waiting for I/O (UzOS: never)
 *
 * Tools like top, vmstat and mpstat read this file twice and subtract:
 * the difference over one second IS the CPU usage of that second.
 *
 * Uses: cpu_account_totals / _switches (cpu_management/core/accounting), cpu_task_forks
 *       (cpu_management/core/task), loadavg_runnable (cpu_management/core/loadavg), isr_irq_count
 *       (arch/x86/isr), rtc_read / rtc_unix_time and timer_uptime_ms
 *       (device_management), affinity_cpu_count (cpu_management/core/affinity).
 */

#include "modules/filesystem_management/core/procfs/proc_stat.h"
#include "modules/cpu_management/core/accounting/cpu_account.h"   // cpu_account_*
#include "modules/cpu_management/core/task/cpu_task.h"            // cpu_task_forks
#include "modules/cpu_management/core/loadavg/loadavg.h"          // loadavg_runnable
#include "modules/cpu_management/core/affinity/affinity.h"        // affinity_cpu_count
#include "modules/device_management/rtc/rtc.h"                    // rtc_read, rtc_unix_time
#include "modules/device_management/timer/timer.h"                // timer_uptime_ms
#include "arch/x86/isr/isr.h"                                     // isr_irq_count

// One "cpu" line: the ten columns Linux has today.
static void cpu_line(proc_buf_t *buf, const char *name, const cpu_ticks_t *t) {
    proc_printf(buf, "%s %u %u %u %u 0 0 0 0 0 0\n",
                name, t->user, t->nice, t->system, t->idle);
}

void proc_stat(proc_buf_t *buf) {
    cpu_ticks_t t;
    cpu_account_totals(&t);

    cpu_line(buf, "cpu ", &t);
    for (int n = 0; n < affinity_cpu_count(); n++)
        cpu_line(buf, "cpu0", &t);          // one CPU: its line equals the sum

    proc_printf(buf, "intr %u", cpu_account_interrupts());
    for (int irq = 0; irq < ISR_IRQS; irq++)
        proc_printf(buf, " %u", isr_irq_count(irq));
    proc_printf(buf, "\n");

    proc_printf(buf, "ctxt %u\n", cpu_account_switches());

    rtc_time_t now;
    uint32_t boot = rtc_read(&now) ? rtc_unix_time(&now) - timer_uptime_ms() / 1000 : 0;
    proc_printf(buf, "btime %u\n", boot);
    proc_printf(buf, "processes %u\n", cpu_task_forks());
    proc_printf(buf, "procs_running %u\n", loadavg_runnable());
    proc_printf(buf, "procs_blocked 0\n");
}
