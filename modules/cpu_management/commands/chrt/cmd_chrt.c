/*
 * cmd_chrt.c - `chrt`: see and set the scheduling policy.
 *
 *   chrt -m                    each policy with its priority range
 *   chrt -p 7                  policy and priority of process 7
 *   chrt -p 20 7               process 7: SCHED_RR (the default), priority 20
 *   chrt -f -p 10 7            process 7: SCHED_FIFO, priority 10
 *   chrt -o -p 0 7             process 7: back to SCHED_OTHER
 *   chrt -f 30 spawn           run spawn as SCHED_FIFO 30 (a new process)
 *   chrt -b 0 ls               run ls as SCHED_BATCH
 *
 *   policies:  -o other   -f fifo   -r rr   -b batch   -i idle
 *   -R         reset-on-fork: children start as SCHED_OTHER again
 *
 * Every change goes through policy_set(): the defence priority_bounds
 * decides (real-time needs root or RLIMIT_RTPRIO).
 *
 * Uses: policy_* (core/policy), chrt_run_child (run_child.c, this folder), the process
 *       table (process_management), ui_* (ui), arg_next / atoi /
 *       strcmp (lib).
 */

#include "modules/cpu_management/commands/chrt/cmd_chrt.h"          // this command
#include "modules/cpu_management/core/policy/policy.h"              // policy_*
#include "modules/cpu_management/commands/chrt/run_child.h"             // chrt_run_child (this folder)
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strlen

// What the options asked for.
typedef struct {
    int policy;             // -1 = not given
    int reset_on_fork;
    int priority;
} chrt_request_t;

static void show_ranges(void) {
    static const int all[] = { SCHED_OTHER, SCHED_FIFO, SCHED_RR, SCHED_BATCH, SCHED_IDLE };
    for (unsigned i = 0; i < sizeof(all) / sizeof(all[0]); i++)
        ui_printf("%-12s min/max priority\t: %d/%d\n", policy_name(all[i]),
                  policy_priority_min(all[i]), policy_priority_max(all[i]));
    vga_print_str("SCHED_DEADLINE not supported\n");
}

static void show_process(const process_t *p) {
    ui_printf("pid %u's current scheduling policy: %s%s\n", p->pid, policy_name(p->policy),
              p->reset_on_fork ? "|SCHED_RESET_ON_FORK" : "");
    ui_printf("pid %u's current scheduling priority: %d\n", p->pid, p->rt_priority);
}

// Runs before the child may run (run_child.c, this folder).
static int set_child_policy(process_t *child, void *arg) {
    chrt_request_t *r = arg;
    if (!policy_set(scheduler_get_current_process(), child->pid, r->policy, r->priority,
                    r->reset_on_fork)) {
        ui_error("chrt", "failed to set policy: refused (range, or real-time needs root)");
        return 0;           // unlike nice, chrt does not run the command then
    }
    return 1;
}

static int option_policy(const char *word) {
    if (strcmp(word, "-o") == 0 || strcmp(word, "--other") == 0) return SCHED_OTHER;
    if (strcmp(word, "-f") == 0 || strcmp(word, "--fifo") == 0)  return SCHED_FIFO;
    if (strcmp(word, "-r") == 0 || strcmp(word, "--rr") == 0)    return SCHED_RR;
    if (strcmp(word, "-b") == 0 || strcmp(word, "--batch") == 0) return SCHED_BATCH;
    if (strcmp(word, "-i") == 0 || strcmp(word, "--idle") == 0)  return SCHED_IDLE;
    if (strcmp(word, "-d") == 0 || strcmp(word, "--deadline") == 0) return SCHED_DEADLINE;
    return -1;
}

void cmd_chrt(const char *args) {
    chrt_request_t r = { -1, 0, 0 };
    int pid_mode = 0;
    char word[24];
    const char *rest = args;
    const char *numbers[2];             // up to two plain numbers: priority, pid
    char number_text[2][24];
    int count = 0;

    // Options first; the first word that is not an option ends them.
    while (1) {
        const char *next = arg_next(rest, word, sizeof(word));
        if (!word[0])
            break;
        int policy = option_policy(word);
        if (policy >= 0) {
            r.policy = policy;
        } else if (strcmp(word, "-R") == 0 || strcmp(word, "--reset-on-fork") == 0) {
            r.reset_on_fork = 1;
        } else if (strcmp(word, "-p") == 0 || strcmp(word, "--pid") == 0) {
            pid_mode = 1;
        } else if (strcmp(word, "-m") == 0 || strcmp(word, "--max") == 0) {
            show_ranges();
            return;
        } else if (word[0] >= '0' && word[0] <= '9' && count < 2) {
            strcpy(number_text[count], word);
            numbers[count] = number_text[count];
            count++;
            if (!pid_mode) {            // "chrt -f 30 command": the rest is the command
                rest = next;
                break;
            }
        } else {
            break;                      // a command word
        }
        rest = next;
    }
    if (r.policy == SCHED_DEADLINE) {
        ui_error("chrt", "SCHED_DEADLINE is not supported by UzOS");
        return;
    }

    if (pid_mode) {
        // chrt -p PID  /  chrt [policy] -p PRIO PID
        if (count == 0) {
            ui_usage("chrt -p [policy] [prio] PID");
            return;
        }
        process_t *p = process_get_by_pid(atoi(numbers[count - 1]));
        if (!p) {
            ui_error("chrt", "no such process");
            return;
        }
        if (count == 1 && r.policy < 0) {
            show_process(p);
            return;
        }
        r.priority = count == 2 ? atoi(numbers[0]) : 0;
        if (r.policy < 0)
            r.policy = SCHED_RR;        // chrt's default policy
        if (!policy_set(scheduler_get_current_process(), p->pid, r.policy, r.priority,
                        r.reset_on_fork)) {
            ui_error("chrt", "failed to set policy: refused (range, system process, or real-time needs root)");
            return;
        }
        show_process(p);
        return;
    }

    // chrt [policy] PRIO command
    while (*rest == ' ')
        rest++;
    if (count != 1 || !*rest) {
        ui_usage("chrt [-f|-r|-o|-b|-i] [-R] prio command  |  chrt -p [prio] PID  |  chrt -m");
        return;
    }
    r.priority = atoi(numbers[0]);
    if (r.policy < 0)
        r.policy = SCHED_RR;
    if (chrt_run_child(rest, set_child_policy, &r) < 0)
        ui_error("chrt", "the command did not run");
}
