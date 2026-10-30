/*
 * cmd_wait.c - `wait [%job | pid]...`: wait until jobs end.
 *
 *   spawn ; spawn ; wait       both workers end (9 s), then:
 *   [1] worker: exit code 0
 *   [2] worker: exit code 0
 *
 * Without arguments: every job of the shell. Ctrl+C stops waiting (the
 * jobs keep running). How each one ended comes from its exit record
 * (core/exitlog), the same that wait4() reads.
 *
 * Uses: process_get_table / process_get_by_pid (core/process),
 *       scheduler_get_current_process (core/scheduler), exitlog_take
 *       (core/exitlog), signal_pending / signal_name (core/signal),
 *       sys_sleep / sys_sleep_interruptible (syscalls/sleep), arg_next, atoi, ui.
 */

#include "modules/process_management/commands/wait/cmd_wait.h"       // this file
#include "modules/process_management/core/process/process.h"          // process_get_table, process_get_by_pid
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"            // signal_pending, signal_name
#include "modules/process_management/core/exitlog/exitlog.h"          // exitlog_take
#include "modules/process_management/syscalls/sleep/sys_sleep.h"      // sys_sleep, sys_sleep_interruptible
#include "lib/args/args.h"                                            // arg_next
#include "lib/stdlib/stdlib.h"                                        // atoi
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_error

#define WAIT_STEP_MS  50
#define RECORD_TRIES  20
#define MAX_WAIT      MAX_PROCESSES

static int is_job(const process_t *p, const process_t *shell) {
    return p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
           p->ppid == shell->pid && (!p->tgid || p->tgid == p->pid) && !p->is_system;
}

// The pid of job number 'n' of 'shell' (1 = the oldest), or 0.
static uint32_t job_pid(const process_t *shell, int n) {
    process_t *table = process_get_table();
    uint32_t last = 0;
    for (int k = 1; ; k++) {
        process_t *job = 0;
        for (int i = 0; i < MAX_PROCESSES; i++)
            if (is_job(&table[i], shell) && table[i].pid > last && (!job || table[i].pid < job->pid))
                job = &table[i];
        if (!job)
            return 0;
        last = job->pid;
        if (k == n)
            return job->pid;
    }
}

// Wait for 'pid' to end and say how. 0 = Ctrl+C came first.
static int wait_one(process_t *self, uint32_t pid, int label) {
    for (process_t *p; (p = process_get_by_pid(pid)) && p->state != PROCESS_STATE_ZOMBIE; )
        if (!sys_sleep_interruptible(WAIT_STEP_MS) && signal_pending(self, SIGINT))
            return 0;

    exit_record_t r;
    for (int i = 0; i < RECORD_TRIES; i++) {
        if (exitlog_take(self->pid, (int32_t)pid, 0, &r)) {
            ui_printf("[%d] PID %u: ", label, pid);
            if (r.status & 0x7F)
                ui_printf("ended by SIG%s\n", signal_name(r.status & 0x7F));
            else
                ui_printf("exit code %d\n", (r.status >> 8) & 0xFF);
            return 1;
        }
        sys_sleep(WAIT_STEP_MS);
    }
    ui_printf("[%d] PID %u: ended\n", label, pid);
    return 1;
}

void cmd_wait(const char *args) {
    process_t *self = scheduler_get_current_process();
    uint32_t pids[MAX_WAIT];
    int labels[MAX_WAIT], count = 0;
    char word[12];

    for (const char *rest = arg_next(args, word, sizeof(word)); word[0] && count < MAX_WAIT;
         rest = arg_next(rest, word, sizeof(word))) {
        uint32_t pid = word[0] == '%' ? job_pid(self, atoi(word + 1)) : (uint32_t)atoi(word);
        process_t *p = pid ? process_get_by_pid(pid) : 0;
        if (!p || p->ppid != self->pid) {
            ui_printf("wait: %s is not a job of this shell\n", word);
            continue;
        }
        labels[count] = word[0] == '%' ? atoi(word + 1) : count + 1;
        pids[count++] = pid;
    }
    if (!args[0])                               // no arguments: every job
        for (uint32_t pid; count < MAX_WAIT && (pid = job_pid(self, count + 1)); count++) {
            pids[count] = pid;
            labels[count] = count + 1;
        }

    for (int i = 0; i < count; i++)
        if (!wait_one(self, pids[i], labels[i]))
            return;                             // Ctrl+C: the shell prints ^C
}
