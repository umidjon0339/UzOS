/*
 * cmd_jobs.c - `jobs`: the programs this shell started that still exist.
 *
 *   [1]   Running   worker (PID 5)
 *   [2]-  Stopped   hello (PID 7)
 *   [3]+  Running   pause (PID 9)
 *
 * A job is a child of the shell's process (spawn, exec &, nohup, a
 * stopped `exec`), numbered in the order they were started. `+` is the
 * newest (what fg/bg take by default), `-` the one before. There is no
 * job table: the process table already knows every child and its state.
 *
 * Uses: process_get_table (core/process), scheduler_get_current_process
 *       (core/scheduler), ui.
 */

#include "modules/process_management/commands/jobs/cmd_jobs.h"       // this file
#include "modules/process_management/core/process/process.h"          // process_get_table
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_color

// Is 'p' one of 'shell's jobs? (Its child, alive, not a thread, not the OS's own.)
static int is_job(const process_t *p, const process_t *shell) {
    return p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
           p->ppid == shell->pid && (!p->tgid || p->tgid == p->pid) && !p->is_system;
}

void cmd_jobs(const char *args) {
    (void)args;
    process_t *shell = scheduler_get_current_process();
    process_t *table = process_get_table();

    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        count += is_job(&table[i], shell);
    if (!count) {
        ui_printf("(no jobs)\n");
        return;
    }

    uint32_t last = 0;
    for (int n = 1; n <= count; n++) {
        process_t *job = 0;                     // the next one by pid
        for (int i = 0; i < MAX_PROCESSES; i++)
            if (is_job(&table[i], shell) && table[i].pid > last &&
                (!job || table[i].pid < job->pid))
                job = &table[i];
        last = job->pid;

        int stopped = job->state == PROCESS_STATE_STOPPED;
        ui_printf("[%d]%c  ", n, n == count ? '+' : n == count - 1 ? '-' : ' ');
        ui_color(stopped ? UI_WARNING : UI_GOOD);
        ui_printf("%-9s", stopped ? "Stopped" : "Running");
        ui_reset();
        ui_printf(" %s (PID %u)\n", job->name, job->pid);
    }
}
