/*
 * cmd_nice.c - `nice [-n N] [command]`: start a command with a different nice.
 *
 *   nice                   print the niceness of the shell (0)
 *   nice ls                ls with nice 10 (the default step)
 *   nice -n 5 spawn        spawn with nice 5: the worker inherits it
 *   nice -n -10 top        top with nice -10: only root may go below 0
 *   nice -5 ls             old syntax for -n 5
 *
 * The command runs as a NEW process (run_child.c, this folder) whose nice is changed
 * before it may run; the shell itself stays at its own nice. If the
 * change is refused (a user asking for less than 0), the command still
 * runs, unchanged, after a warning: that is what Linux's nice does.
 *
 * Uses: nice_run_child (run_child.c, this folder), priority_set_nice / NICE_* (core/priority),
 *       the current process (process_management), ui_* (ui),
 *       arg_next / atoi / strcmp (lib).
 */

#include "modules/cpu_management/commands/nice/cmd_nice.h"          // this command
#include "modules/cpu_management/commands/nice/run_child.h"             // nice_run_child (this folder)
#include "modules/cpu_management/core/priority/priority.h"          // priority_set_nice, NICE_*
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp

#define DEFAULT_STEP 10     // `nice command` = nice 10 more than now

static int clamp(int nice) {
    return nice < NICE_MIN ? NICE_MIN : nice > NICE_MAX ? NICE_MAX : nice;
}

// Runs before the child may run (run_child.c, this folder): give it its new nice.
static int set_child_nice(process_t *child, void *arg) {
    int nice = *(int *)arg;
    if (!priority_set_nice(scheduler_get_current_process(), child->pid, nice))
        ui_warn("nice", "cannot set niceness: permission denied (running it anyway)");
    return 1;
}

static int is_number(const char *s) {
    if (*s == '-' || *s == '+')
        s++;
    if (!*s)
        return 0;
    for (; *s; s++)
        if (*s < '0' || *s > '9')
            return 0;
    return 1;
}

void cmd_nice(const char *args) {
    process_t *self = scheduler_get_current_process();
    char word[16];
    int step = DEFAULT_STEP;

    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0]) {
        ui_printf("%d\n", self->nice);      // the shell's own niceness
        return;
    }
    if (strcmp(word, "-n") == 0) {          // nice -n N command
        rest = arg_next(rest, word, sizeof(word));
        if (!is_number(word)) {
            ui_usage("nice [-n N] command");
            return;
        }
        step = atoi(word);
    } else if (word[0] == '-' && is_number(word + 1)) {
        step = atoi(word + 1);              // nice -5 command
    } else {
        rest = args;                        // no option: the whole line is the command
    }

    while (*rest == ' ')
        rest++;
    if (!*rest) {
        ui_usage("nice [-n N] command");
        return;
    }

    int nice = clamp(self->nice + step);
    if (nice_run_child(rest, set_child_nice, &nice) < 0)
        ui_error("nice", "cannot run that command (unknown, or no free process slot)");
}
