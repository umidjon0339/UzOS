/*
 * cmd_renice.c - `renice`: change the nice value of running processes.
 *
 *   renice 10 -p 7 8        processes 7 and 8 get nice 10
 *   renice -n 5 -p 7        the same with -n
 *   renice 19 -u guest      every process of user guest
 *
 * For each one it prints what changed, as Linux does:
 *
 *   7 (process ID) old priority 0, new priority 10
 *
 * The rules are the defence priority_bounds (via setpriority): users may
 * only renice their own processes, and only upward (nicer).
 *
 * Uses: sys_setpriority / sys_getpriority (syscalls), users_find_name
 *       (user_management), ui_* (ui), arg_next / atoi / strcmp (lib).
 */

#include "modules/cpu_management/commands/renice/cmd_renice.h"           // this command
#include "modules/cpu_management/syscalls/setpriority/sys_setpriority.h" // sys_setpriority, PRIO_*
#include "modules/cpu_management/syscalls/getpriority/sys_getpriority.h" // sys_getpriority
#include "modules/user_management/core/users/users.h"                    // users_find_name
#include "modules/ui/core/style/ui.h"                                    // ui_*
#include "lib/args/args.h"                                               // arg_next
#include "lib/stdlib/stdlib.h"                                           // atoi
#include "lib/string/string.h"                                           // strcmp

static const char *kind_name(int which) {
    return which == PRIO_USER ? "user ID" : "process ID";
}

// Renice one process or user; print the result line.
static void renice_one(int which, uint32_t who, int nice) {
    int32_t before = sys_getpriority(which, who);
    if (before < 0) {
        ui_start(UI_ERROR, "renice");
        ui_printf("%u: no such %s", who, which == PRIO_USER ? "user's process" : "process");
        ui_end();
        return;
    }
    if (sys_setpriority(which, who, nice) < 0) {
        ui_start(UI_ERROR, "renice");
        ui_printf("failed to set priority for %u (%s): permission denied", who, kind_name(which));
        ui_end();
        return;
    }
    ui_printf("%u (%s) old priority %d, new priority %d\n", who, kind_name(which),
              20 - before, 20 - sys_getpriority(which, who));
}

void cmd_renice(const char *args) {
    char word[24];
    int which = PRIO_PROCESS, have_nice = 0, did = 0, nice = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-n") == 0 || strcmp(word, "--priority") == 0) {
            args = arg_next(args, word, sizeof(word));
            nice = atoi(word);
            have_nice = 1;
        } else if (strcmp(word, "-p") == 0) {
            which = PRIO_PROCESS;
        } else if (strcmp(word, "-u") == 0) {
            which = PRIO_USER;
        } else if (strcmp(word, "-g") == 0) {
            ui_error("renice", "UzOS has no process groups (use -p or -u)");
            return;
        } else if (!have_nice) {
            nice = atoi(word);
            have_nice = 1;
        } else if (which == PRIO_USER && (word[0] < '0' || word[0] > '9')) {
            const user_t *u = users_find_name(word);
            if (!u) {
                ui_error("renice", "no such user");
                continue;
            }
            renice_one(PRIO_USER, u->uid, nice);
            did = 1;
        } else {
            renice_one(which, atoi(word), nice);
            did = 1;
        }
    }
    if (!did)
        ui_usage("renice [-n] N [-p] PID... | renice [-n] N -u USER...");
}
