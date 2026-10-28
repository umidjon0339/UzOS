/*
 * cmd_systemctl.c - `systemctl`: the services, and their resource settings.
 *
 *   systemctl                    list the units (= list-units)
 *   systemctl status statusd     one service: state, PID, CPU used, policy
 *   systemctl show statusd       its settings as Name=value lines
 *
 * On Linux, systemd starts the services and keeps one "unit" for each.
 * UzOS's services are its SYSTEM processes (process_t.is_system): the
 * idle task, kernel_main (the shell) and statusd (the status bar). Each
 * one is shown as <name>.service. The resource settings (Nice,
 * CPUSchedulingPolicy, CPUAffinity, Limit*) are the real values from
 * the process table, under the names systemd uses for them.
 *
 * Starting and stopping is not offered: system processes cannot be
 * killed (kill_guard) and are started by the kernel itself.
 *
 * Uses: the process table (process_management), policy_* (core/policy),
 *       affinity_* (core/affinity), cpu_account_cycles (core/accounting),
 *       tsc_cycles_to_us (core/tsc), rlimit_* (core/rlimit), thread_count
 *       (in this file), timer_* / rtc_read (device_management), ui_* (ui),
 *       arg_next / format / string (lib).
 */

#include "modules/cpu_management/commands/systemctl/cmd_systemctl.h"    // this command
#include "modules/cpu_management/core/policy/policy.h"                  // policy_name
#include "modules/cpu_management/core/affinity/affinity.h"              // affinity_*
#include "modules/cpu_management/core/accounting/cpu_account.h"         // cpu_account_cycles
#include "modules/cpu_management/core/tsc/tsc.h"                        // tsc_cycles_to_us
#include "modules/cpu_management/core/rlimit/rlimit.h"                  // RLIMIT_*
#include "modules/process_management/core/process/process.h"            // process table
#include "modules/device_management/timer/timer.h"                      // timer_*
#include "modules/device_management/rtc/rtc.h"                          // rtc_read
#include "modules/ui/core/style/ui.h"                                   // ui_*
#include "lib/args/args.h"                                              // arg_next
#include "lib/format/format.h"                                          // format
#include "lib/string/string.h"                                          // strcmp, strlen, strncmp

// Threads alive in thread group 'tgid' (every clone(CLONE_THREAD) child
// has its parent's tgid). Only this file needs it, so it lives here.
static int thread_count(uint32_t tgid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].tgid == tgid && table[i].state != PROCESS_STATE_FREE &&
            table[i].state != PROCESS_STATE_ZOMBIE)
            n++;
    return n;
}

static const struct {
    const char *name;
    const char *description;
} descriptions[] = {
    { "idle",        "Idle task: halts an unused CPU" },     // at most 34 columns:
    { "kernel_main", "Kernel's first process: the shell" },  // list-units has no room
    { "statusd",     "Status bar daemon" },                  // for more
};

static const char *description_of(const char *name) {
    for (unsigned i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); i++)
        if (strcmp(descriptions[i].name, name) == 0)
            return descriptions[i].description;
    return "UzOS system process";
}

// "statusd", "statusd.service" or a PID -> the process; NULL if none.
static process_t *find_unit(const char *unit) {
    char name[PROCESS_NAME_LEN];
    int len = strlen(unit);
    format(name, sizeof(name), "%s", unit);
    if (len > 8 && strcmp(unit + len - 8, ".service") == 0)
        name[len - 8] = '\0';

    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].is_system &&
            strcmp(table[i].name, name) == 0)
            return &table[i];
    return 0;
}

static void list_units(void) {
    process_t *table = process_get_table();
    int n = 0;
    ui_color(UI_TITLE);
    vga_print_str("  UNIT                 LOAD   ACTIVE SUB     DESCRIPTION\n");
    ui_reset();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || !p->is_system)
            continue;
        char unit[40];
        format(unit, sizeof(unit), "%s.service", p->name);
        ui_printf("  %-20s loaded ", unit);
        ui_color(UI_GOOD);
        vga_print_str("active running ");
        ui_reset();
        ui_printf("%s\n", description_of(p->name));
        n++;
    }
    ui_printf("\n%d loaded units listed.\n", n);
}

static void cpu_time(const process_t *p, char *out, int size) {
    uint32_t ms = tsc_cycles_to_us(cpu_account_cycles(p)) / 1000;
    if (ms >= 1000)
        format(out, size, "%u.%03us", ms / 1000, ms % 1000);
    else
        format(out, size, "%ums", ms);
}

static void status(process_t *p) {
    char text[48];
    uint32_t hz = timer_frequency() ? timer_frequency() : 1;
    uint32_t ago = (timer_ticks - p->start_tick) / hz;

    ui_color(UI_GOOD);
    vga_print_str("* ");
    ui_reset();
    ui_printf("%s.service - %s\n", p->name, description_of(p->name));
    ui_printf("     Loaded: loaded (built into the kernel)\n");
    ui_printf("     Active: ");
    ui_color(UI_GOOD);
    vga_print_str("active (running)");
    ui_reset();
    ui_printf(" for %u min %u s\n", ago / 60, ago % 60);
    ui_printf("   Main PID: %u (%s)\n", p->pid, p->name);
    ui_printf("      Tasks: %d\n", thread_count(p->tgid ? p->tgid : p->pid));
    cpu_time(p, text, sizeof(text));
    ui_printf("        CPU: %s\n", text);
    ui_printf("     Policy: %s, nice %d", policy_name(p->policy), p->nice);
    if (policy_is_realtime(p->policy))
        ui_printf(", rt priority %d", p->rt_priority);
    affinity_list(affinity_get(p), text, sizeof(text));
    ui_printf("\n   Affinity: CPU %s\n", text);
}

static void limit_line(const char *name, uint32_t value) {
    if (value == RLIM_INFINITY)
        ui_printf("%s=infinity\n", name);
    else
        ui_printf("%s=%u\n", name, value);
}

static void show(process_t *p) {
    char text[32];
    ui_printf("Id=%s.service\nDescription=%s\n", p->name, description_of(p->name));
    ui_printf("ActiveState=active\nSubState=running\nMainPID=%u\n", p->pid);
    ui_printf("CPUUsageNSec=%u000\n", tsc_cycles_to_us(cpu_account_cycles(p)));
    ui_printf("TasksCurrent=%d\n", thread_count(p->tgid ? p->tgid : p->pid));
    ui_printf("Nice=%d\n", p->nice);
    format(text, sizeof(text), "%s", policy_name(p->policy) + 6);   // "SCHED_FIFO" -> "FIFO"
    for (char *c = text; *c; c++)
        if (*c >= 'A' && *c <= 'Z')
            *c += 'a' - 'A';                                       // systemd writes "fifo"
    ui_printf("CPUSchedulingPolicy=%s\nCPUSchedulingPriority=%d\n", text, p->rt_priority);
    affinity_list(affinity_get(p), text, sizeof(text));
    ui_printf("CPUAffinity=%s\n", text);
    limit_line("LimitCPU", p->rlim_cur[RLIMIT_CPU]);
    limit_line("LimitNOFILE", p->rlim_cur[RLIMIT_NOFILE]);
    limit_line("LimitNPROC", p->rlim_cur[RLIMIT_NPROC]);
    limit_line("LimitNICE", p->rlim_cur[RLIMIT_NICE]);
    limit_line("LimitRTPRIO", p->rlim_cur[RLIMIT_RTPRIO]);
}

void cmd_systemctl(const char *args) {
    char verb[24], unit[40];
    const char *rest = arg_next(args, verb, sizeof(verb));
    arg_next(rest, unit, sizeof(unit));

    if (!verb[0] || strcmp(verb, "list-units") == 0) {
        list_units();
        return;
    }
    if ((strcmp(verb, "status") == 0 || strcmp(verb, "show") == 0) && unit[0]) {
        process_t *p = find_unit(unit);
        if (!p) {
            ui_error("systemctl", "unit not found (systemctl list-units)");
            return;
        }
        if (verb[1] == 't')
            status(p);
        else
            show(p);
        return;
    }
    if (strcmp(verb, "start") == 0 || strcmp(verb, "stop") == 0 || strcmp(verb, "restart") == 0) {
        ui_error("systemctl", "UzOS's services are started by the kernel and cannot be stopped");
        return;
    }
    ui_usage("systemctl [list-units | status UNIT | show UNIT]");
}
