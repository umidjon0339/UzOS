/*
 * cmd_fg.c - `fg [%job]`: bring a job to the foreground.
 *
 *   exec hello   ... Ctrl+Z        hello is stopped, the prompt is back
 *   fg                             hello carries on; the shell waits
 *                                  Ctrl+C ends it, Ctrl+Z stops it again
 *
 * The job (default: the newest, `+` in `jobs`) gets SIGCONT if it was
 * stopped. When it ends, its exit record (core/exitlog) says how.
 *
 * Uses: process_get_table / process_get_by_pid / process_kill
 *       (core/process), scheduler_get_current_process (core/scheduler),
 *       signal_send / signal_pending / signal_clear / signal_name
 *       (core/signal), exitlog_take (core/exitlog),
 *       sys_sleep / sys_sleep_interruptible (syscalls/sleep), atoi, ui.
 */

#include "modules/process_management/commands/fg/cmd_fg.h"           // this file
#include "modules/process_management/core/process/process.h"          // process_get_table, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"            // signal_send, signal_pending, SIGCONT
#include "modules/process_management/core/exitlog/exitlog.h"          // exitlog_take
#include "modules/process_management/syscalls/sleep/sys_sleep.h"      // sys_sleep, sys_sleep_interruptible
#include "lib/stdlib/stdlib.h"                                        // atoi
#include "lib/string/string.h"                                        // strncpy
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_error

#define WAIT_STEP_MS  50
#define RECORD_TRIES  20        // up to 1 s for the reaper to write the record

static int is_job(const process_t *p, const process_t *shell) {
    return p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
           p->ppid == shell->pid && (!p->tgid || p->tgid == p->pid) && !p->is_system;
}

// Job 'spec' ("%2", "2", or "" = the newest) of 'shell'; its number in *n.
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
        if (!want || k == want) {
            found = job;
            *n = k;
        }
    }
    return found;
}

static void report(int n, const char *name, int status) {
    ui_start(UI_INFO, "fg");
    if (status & 0x7F)
        ui_printf("[%d] %s ended by SIG%s%s", n, name, signal_name(status & 0x7F),
                  status & 0x80 ? " (core dumped)" : "");
    else
        ui_printf("[%d] %s done, exit code %d", n, name, (status >> 8) & 0xFF);
    ui_end();
}

void cmd_fg(const char *args) {
    process_t *self = scheduler_get_current_process();
    int n = 0;
    process_t *job = find_job(self, args, &n);
    if (!job) {
        ui_error("fg", "no such job (see `jobs`)");
        return;
    }
    uint32_t pid = job->pid;
    char name[PROCESS_NAME_LEN];
    strncpy(name, job->name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    ui_printf("%s\n", name);
    if (job->state == PROCESS_STATE_STOPPED)
        signal_send(job, SIGCONT);

    for (process_t *p; (p = process_get_by_pid(pid)) && p->state != PROCESS_STATE_ZOMBIE; ) {
        if (sys_sleep_interruptible(WAIT_STEP_MS))
            continue;
        if (signal_pending(self, SIGTSTP)) {            // Ctrl+Z: stop it again
            signal_clear(self, SIGTSTP);
            signal_send(p, SIGSTOP);
            ui_printf("\n[%d]+  Stopped   %s (PID %u)\n", n, name, pid);
            return;
        }
        if (signal_pending(self, SIGINT)) {             // Ctrl+C: end it
            process_kill(pid);
            break;
        }
    }

    exit_record_t r;
    for (int i = 0; i < RECORD_TRIES; i++) {
        if (exitlog_take(self->pid, (int32_t)pid, 0, &r)) {
            report(n, name, r.status);
            return;
        }
        sys_sleep(WAIT_STEP_MS);
    }
}
