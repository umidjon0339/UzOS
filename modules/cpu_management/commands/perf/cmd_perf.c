/*
 * cmd_perf.c - `perf`: one command, several tools.
 *
 *   perf stat command      count what a command did     (perf_stat/)
 *   perf record command    sample where time goes        (perf_record/)
 *   perf report            show what record found        (perf_report/)
 *   perf list              the events UzOS can count     (here)
 *
 * This file only reads the first word and hands the rest of the line
 * to the tool, as Linux's perf does.
 *
 * Uses: cmd_perf_stat / cmd_perf_record / cmd_perf_report (their
 *       folders), ui_* (ui), arg_next / strcmp (lib).
 */

#include "modules/cpu_management/commands/perf/cmd_perf.h"                  // this command
#include "modules/cpu_management/commands/perf_stat/cmd_perf_stat.h"        // perf stat
#include "modules/cpu_management/commands/perf_record/cmd_perf_record.h"    // perf record
#include "modules/cpu_management/commands/perf_report/cmd_perf_report.h"    // perf report
#include "modules/ui/core/style/ui.h"                                       // ui_*
#include "lib/args/args.h"                                                  // arg_next
#include "lib/string/string.h"                                              // strcmp

// The events of perf_event_open() (syscalls/perf_event_open/perf_counter.c
// counts them). Kept here as a plain table so `perf list` reads on its own.
static const struct {
    const char *name;
    const char *kind;
    int supported;
} events[] = {
    { "cycles",           "Hardware event (TSC)", 1 },
    { "instructions",     "Hardware event",       0 },      // needs a PMU
    { "cpu-clock",        "Software event",       1 },
    { "task-clock",       "Software event",       1 },
    { "page-faults",      "Software event",       1 },
    { "context-switches", "Software event",       1 },
    { "cpu-migrations",   "Software event",       1 },
};

static void list(void) {
    for (unsigned i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
        ui_color(events[i].supported ? ui_text_fg() : UI_DIM);
        ui_printf("  %-30s [%s]%s\n", events[i].name, events[i].kind,
                  events[i].supported ? "" : "  not supported (no PMU)");
    }
    ui_reset();
}

void cmd_perf(const char *args) {
    char tool[16];
    const char *rest = arg_next(args, tool, sizeof(tool));
    while (*rest == ' ')
        rest++;

    if (strcmp(tool, "stat") == 0)
        cmd_perf_stat(rest);
    else if (strcmp(tool, "record") == 0)
        cmd_perf_record(rest);
    else if (strcmp(tool, "report") == 0)
        cmd_perf_report(rest);
    else if (strcmp(tool, "list") == 0)
        list();
    else
        ui_usage("perf stat command | perf record [-o file] command | perf report [-i file] [--sort comm] | perf list");
}
