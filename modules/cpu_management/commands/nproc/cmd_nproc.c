/*
 * cmd_nproc.c - `nproc [--all] [--ignore=N]`: how many CPUs may I use?
 *
 *   nproc              CPUs in the caller's affinity mask (what a program
 *                      should use for "how many worker threads?")
 *   nproc --all        CPUs the system has, whatever the mask says
 *   nproc --ignore=N   N fewer (but never less than 1)
 *
 * Uses: affinity_get / affinity_cpu_count (core/affinity), the current
 *       process (process_management), ui_* (ui), atoi / strncmp (lib).
 */

#include "modules/cpu_management/commands/nproc/cmd_nproc.h"        // this command
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_get, affinity_cpu_count
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_printf, ui_usage
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strncmp

static int count_bits(uint32_t mask) {
    int n = 0;
    for (; mask; mask >>= 1)
        n += mask & 1;
    return n;
}

void cmd_nproc(const char *args) {
    char word[24];
    int all = 0, ignore = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "--all") == 0) {
            all = 1;
        } else if (strncmp(word, "--ignore=", 9) == 0) {
            ignore = atoi(word + 9);
        } else {
            ui_usage("nproc [--all] [--ignore=N]");
            return;
        }
    }

    int n = all ? affinity_cpu_count()
                : count_bits(affinity_get(scheduler_get_current_process()));
    n -= ignore;
    ui_printf("%d\n", n < 1 ? 1 : n);
}
