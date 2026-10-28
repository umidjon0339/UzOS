/*
 * proc_cpuinfo.c - /proc/cpuinfo: the CPU, as Linux describes it.
 *
 *     processor       : 0
 *     vendor_id       : GenuineIntel
 *     cpu family      : 6
 *     model           : 6
 *     model name      : QEMU Virtual CPU version 2.5+
 *     cpu MHz         : 2686.000
 *     flags           : fpu tsc msr pse ...
 *     bogomips        : 5372.00
 *
 * One block per CPU; programs count the "processor" lines to learn
 * how many CPUs there are. "key<TAB>: value" so tools can split lines.
 *
 * Uses: cpu_info / cpu_has / cpu_feature_name (cpu_management/core/info), tsc_mhz
 *       (cpu_management/core/tsc), affinity_cpu_count (cpu_management/core/affinity), cpuid_leaf
 *       (arch/x86/cpuid), cpu_read_cr0 (arch/x86/cpu).
 */

#include "modules/filesystem_management/core/procfs/proc_cpuinfo.h"
#include "modules/cpu_management/core/info/cpu_info.h"          // cpu_info, cpu_has
#include "modules/cpu_management/core/tsc/tsc.h"                // tsc_mhz
#include "modules/cpu_management/core/affinity/affinity.h"      // affinity_cpu_count
#include "arch/x86/cpuid/cpuid.h"                               // cpuid_leaf
#include "arch/x86/cpu/cpu.h"                                   // cpu_read_cr0, CR0_WP

void proc_cpuinfo(proc_buf_t *buf) {
    const cpu_info_t *cpu = cpu_info();
    uint32_t regs[4] = { 0 };
    if (cpuid_available())
        cpuid_leaf(0, regs);                    // EAX = the highest cpuid leaf

    for (int n = 0; n < affinity_cpu_count(); n++) {
        proc_printf(buf, "processor\t: %d\n", n);
        proc_printf(buf, "vendor_id\t: %s\n", cpu->vendor);
        proc_printf(buf, "cpu family\t: %u\n", cpu->family);
        proc_printf(buf, "model\t\t: %u\n", cpu->model);
        proc_printf(buf, "model name\t: %s\n", cpu->brand[0] ? cpu->brand : "unknown");
        proc_printf(buf, "stepping\t: %u\n", cpu->stepping);
        proc_printf(buf, "cpu MHz\t\t: %u.000\n", tsc_mhz());
        proc_printf(buf, "physical id\t: 0\nsiblings\t: 1\ncore id\t\t: %d\ncpu cores\t: 1\n", n);
        proc_printf(buf, "apicid\t\t: %d\n", n);
        proc_printf(buf, "fpu\t\t: %s\n", cpu_has(CPU_FEAT_FPU) ? "yes" : "no");
        proc_printf(buf, "cpuid level\t: %u\n", regs[0]);
        proc_printf(buf, "wp\t\t: %s\n", cpu_read_cr0() & CR0_WP ? "yes" : "no");

        proc_printf(buf, "flags\t\t:");
        for (int f = 0; f < CPU_FEAT_COUNT; f++)
            if (cpu_has(f))
                proc_printf(buf, " %s", cpu_feature_name(f));
        proc_printf(buf, "\n");

        // BogoMIPS: Linux's rough "how fast does a delay loop spin";
        // on x86 it is 2 x the clock in MHz.
        proc_printf(buf, "bogomips\t: %u.00\n\n", 2 * tsc_mhz());
    }
}
