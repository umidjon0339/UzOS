/*
 * cmd_pidof.c - `pidof name...`: the PIDs of the processes called exactly so.
 *
 *   pidof kernel_main idle     ->  2 1
 *
 * All on one line, newest first, as Linux prints them. Zombies are left out.
 *
 * Uses: process_get_table (core/process), arg_next, strcmp, ui.
 */

#include "modules/process_management/commands/pidof/cmd_pidof.h"     // this file
#include "modules/process_management/core/process/process.h"          // process_get_table
#include "lib/args/args.h"                                            // arg_next
#include "lib/string/string.h"                                        // strcmp
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

void cmd_pidof(const char *args) {
    char name[PROCESS_NAME_LEN];
    const char *rest = arg_next(args, name, sizeof(name));
    if (!name[0]) {
        ui_usage("pidof name...");
        return;
    }

    process_t *table = process_get_table();
    int found = 0;
    for (; name[0]; rest = arg_next(rest, name, sizeof(name))) {
        uint32_t newest = 0xFFFFFFFFu;
        for (;;) {                                  // highest PID below 'newest'
            uint32_t best = 0;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                process_t *p = &table[i];
                if (p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
                    strcmp(p->name, name) == 0 && p->pid < newest && p->pid > best)
                    best = p->pid;
            }
            if (!best)
                break;
            ui_printf(found++ ? " %u" : "%u", best);
            newest = best;
        }
    }
    ui_printf(found ? "\n" : "(no such process)\n");
}
