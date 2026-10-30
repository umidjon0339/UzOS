/*
 * cmd_bg.c - `bg [%job]`: let a stopped job carry on in the background.
 *
 *   exec hello   ... Ctrl+Z     stopped
 *   bg                          [1] hello &   (it runs, the prompt stays)
 *
 * Default: the newest stopped job. It gets SIGCONT; the shell does not
 * wait for it (`fg` would).
 *
 * Uses: process_get_table (core/process), scheduler_get_current_process
 *       (core/scheduler), signal_send (core/signal), atoi, ui.
 */

#include "modules/process_management/commands/bg/cmd_bg.h"           // this file
#include "modules/process_management/core/process/process.h"          // process_get_table
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"            // signal_send, SIGCONT
#include "lib/stdlib/stdlib.h"                                        // atoi
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_error

static int is_job(const process_t *p, const process_t *shell) {
    return p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
           p->ppid == shell->pid && (!p->tgid || p->tgid == p->pid) && !p->is_system;
}

// Job 'spec' ("%2", "2", or "" = the newest STOPPED one); its number in *n.
static process_t *find_job(const process_t *shell, const char *spec, int *n) {
    process_t *table = process_get_table();
    int want = spec[0] == '%' ? atoi(spec + 1) : spec[0] ? atoi(spec) : 0;
    process_t *found = 0;
    uint32_t last = 0;
    for (int k = 1; ; k++) {
        process_t *job = 0;
        for (int i = 0; i < MAX_PROCESSES; i++)
            if (is_job(&table[i], shell) && table[i].pid > last && (!job || table[i].pid < job->pid))
                job = &table[i];
        if (!job)
            break;
        last = job->pid;
        if (want ? k == want : job->state == PROCESS_STATE_STOPPED) {
            found = job;
            *n = k;
        }
    }
    return found;
}

void cmd_bg(const char *args) {
    int n = 0;
    process_t *job = find_job(scheduler_get_current_process(), args, &n);
    if (!job) {
        ui_error("bg", "no such stopped job (see `jobs`)");
        return;
    }
    if (job->state != PROCESS_STATE_STOPPED) {
        ui_printf("bg: job %d is already running\n", n);
        return;
    }
    signal_send(job, SIGCONT);
    ui_printf("[%d] %s (PID %u) &\n", n, job->name, job->pid);
}
