/*
 * cmd_cpupower.c - `cpupower`: clock speed and idle states of the CPU.
 *
 *   cpupower frequency-info    driver, governor, current frequency
 *   cpupower idle-info         which idle states exist, how often used
 *   cpupower info              the energy/performance bias (not here)
 *   cpupower frequency-set ... refused: nothing to set (see below)
 *
 * A real CPU can change its clock (P-states) and how deeply it sleeps
 * when idle (C-states). Linux has drivers for both (cpufreq, cpuidle).
 * The CPU QEMU emulates runs at one fixed speed, and UzOS has no
 * cpufreq driver; the speed shown is MEASURED (core/tsc). The idle state
 * is real: the idle task runs hlt ("C1") or spins ("POLL") depending on
 * the power profile (core/power).
 *
 * Uses: tsc_mhz (core/tsc), power_* (core/power), cpu_idle_ms
 *       (core/accounting), affinity_list (core/affinity), ui_* (ui),
 *       arg_next / strcmp (lib).
 */

#include "modules/cpu_management/commands/cpupower/cmd_cpupower.h"     // this command
#include "modules/cpu_management/core/tsc/tsc.h"                       // tsc_mhz
#include "modules/cpu_management/core/power/power.h"                   // power_*
#include "modules/cpu_management/core/accounting/cpu_account.h"        // cpu_idle_ms
#include "modules/cpu_management/core/affinity/affinity.h"             // affinity_list
#include "modules/ui/core/style/ui.h"                                  // ui_*
#include "lib/args/args.h"                                             // arg_next
#include "lib/string/string.h"                                         // strcmp

static void frequency_info(void) {
    char cpus[16];
    affinity_list(affinity_online_mask(), cpus, sizeof(cpus));

    vga_print_str("analyzing CPU 0:\n");
    vga_print_str("  no or unknown cpufreq driver is active on this CPU\n");
    ui_printf("  CPUs which run at the same hardware frequency: %s\n", cpus);
    ui_printf("  CPUs which need to have their frequency coordinated by software: %s\n", cpus);
    ui_printf("  hardware limits: %u MHz - %u MHz (one fixed speed)\n", tsc_mhz(), tsc_mhz());
    vga_print_str("  available cpufreq governors: Not Available\n");
    ui_printf("  current CPU frequency: %u MHz (measured: TSC cycles against the PIT)\n",
              tsc_mhz());
    vga_print_str("  boost state support:\n    Supported: no\n    Active: no\n");
}

static void idle_info(void) {
    int halt = strcmp(power_idle_state(), "C1") == 0;

    vga_print_str("CPUidle driver: none (the idle task, process_management)\n");
    ui_printf("CPUidle governor: power profile '%s' (powerprofilesctl)\n",
              power_profile_name(power_profile()));
    vga_print_str("analyzing CPU 0:\n\nNumber of idle states: 2\n");
    vga_print_str("Available idle states: POLL C1\n");

    ui_printf("POLL:\n  Flags/Description: CPUIDLE CORE POLL IDLE (spin with pause)\n");
    ui_printf("  Usage: %u\n  Disabled: %s\n", power_idle_entries(0), halt ? "yes" : "no");
    ui_printf("C1:\n  Flags/Description: hlt: stop until the next interrupt\n");
    ui_printf("  Usage: %u\n  Disabled: %s\n", power_idle_entries(1), halt ? "no" : "yes");
    ui_printf("Idle time since boot: %u ms\n", cpu_idle_ms());
}

void cmd_cpupower(const char *args) {
    char verb[24];
    arg_next(args, verb, sizeof(verb));

    if (strcmp(verb, "frequency-info") == 0) {
        frequency_info();
    } else if (strcmp(verb, "idle-info") == 0) {
        idle_info();
    } else if (strcmp(verb, "info") == 0) {
        vga_print_str("System does not support Intel's performance bias setting\n");
    } else if (strcmp(verb, "frequency-set") == 0 || strcmp(verb, "idle-set") == 0) {
        ui_error("cpupower", "no cpufreq/cpuidle driver: use powerprofilesctl to change the idle state");
    } else {
        ui_usage("cpupower frequency-info | idle-info | info");
    }
}
