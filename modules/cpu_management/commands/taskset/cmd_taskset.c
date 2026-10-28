/*
 * cmd_taskset.c - `taskset`: which CPUs a process may run on.
 *
 *   taskset -p 7               pid 7's current affinity mask: 1
 *   taskset -cp 7              pid 7's current affinity list: 0
 *   taskset -p 1 7             set the mask of 7 to 0x1 (CPU 0)
 *   taskset -c -p 0 7          the same, as a CPU list
 *   taskset 1 ls               run ls on the CPUs of mask 0x1
 *   taskset -c 0 ls            run ls on CPU 0
 *
 * Masks are hexadecimal (bit n = CPU n), lists are "0", "0-3", "0,2".
 * With one CPU the only valid choice is CPU 0; asking for CPU 1 is
 * refused, as on a real machine without a CPU 1.
 *
 * Uses: affinity_* (core/affinity), taskset_run_child (run_child.c, this folder), the
 *       process table (process_management), ui_* (ui), arg_next /
 *       strtoul / strcmp (lib).
 */

#include "modules/cpu_management/commands/taskset/cmd_taskset.h"    // this command
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_*
#include "modules/cpu_management/commands/taskset/run_child.h"          // taskset_run_child (this folder)
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // strtoul, atoi
#include "lib/string/string.h"                                      // strcmp

// "1", "0x3" (a mask) or "0-1" (a list, with -c) -> mask; 0 if invalid.
static uint32_t parse_cpus(const char *text, int as_list) {
    if (as_list)
        return affinity_parse_list(text);
    char *end;
    uint32_t mask = strtoul(text, &end, 16);
    return *end ? 0 : mask;
}

static void show(const process_t *p, int as_list, const char *when) {
    if (as_list) {
        char list[32];
        affinity_list(affinity_get(p), list, sizeof(list));
        ui_printf("pid %u's %s affinity list: %s\n", p->pid, when, list);
    } else {
        ui_printf("pid %u's %s affinity mask: %x\n", p->pid, when, affinity_get(p));
    }
}

// Runs before the child may run (run_child.c, this folder).
static int set_child_mask(process_t *child, void *arg) {
    if (!affinity_set(scheduler_get_current_process(), child->pid, *(uint32_t *)arg)) {
        ui_error("taskset", "failed to set affinity: no such CPU");
        return 0;
    }
    return 1;
}

void cmd_taskset(const char *args) {
    int as_list = 0, pid_mode = 0;
    char word[24], first[24] = "", second[24] = "";
    const char *rest = args;

    while (1) {                             // options
        const char *next = arg_next(rest, word, sizeof(word));
        if (strcmp(word, "-c") == 0 || strcmp(word, "--cpu-list") == 0)
            as_list = 1;
        else if (strcmp(word, "-p") == 0 || strcmp(word, "--pid") == 0)
            pid_mode = 1;
        else if (strcmp(word, "-cp") == 0 || strcmp(word, "-pc") == 0)
            as_list = pid_mode = 1;
        else
            break;
        rest = next;
    }
    rest = arg_next(rest, first, sizeof(first));

    if (pid_mode) {
        arg_next(rest, second, sizeof(second));
        const char *pid_text = second[0] ? second : first;
        process_t *p = process_get_by_pid(atoi(pid_text));
        if (!first[0] || !p) {
            ui_error("taskset", "no such process (usage: taskset [-c] -p [cpus] PID)");
            return;
        }
        if (!second[0]) {                   // taskset -p PID: show
            show(p, as_list, "current");
            return;
        }
        uint32_t mask = parse_cpus(first, as_list);
        show(p, as_list, "current");
        if (!mask || !affinity_set(scheduler_get_current_process(), p->pid, mask)) {
            ui_error("taskset", "failed to set affinity: no such CPU, system process, or not yours");
            return;
        }
        show(p, as_list, "new");
        return;
    }

    while (*rest == ' ')
        rest++;
    uint32_t mask = parse_cpus(first, as_list);
    if (!first[0] || !*rest || !mask) {
        ui_usage("taskset [-c] mask|list command  |  taskset [-c] -p [mask|list] PID");
        return;
    }
    if (taskset_run_child(rest, set_child_mask, &mask) < 0)
        ui_error("taskset", "the command did not run");
}
