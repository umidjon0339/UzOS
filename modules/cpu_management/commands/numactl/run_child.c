/*
 * run_child.c - Start the command in a child process and wait for it.
 *
 *   shell (waits)                    child "ls"
 *   numactl_run_child("ls")
 *     process_create  ------------>  (not running yet: interrupts off)
 *     setup(child): give it the memory policy / CPUs
 *     interrupts on  -------------->  runs "ls" through the shell's table
 *     sleep, look, sleep, ...         ...
 *                                     done: ends, the reaper frees it
 *
 * The child inherits the user, limits, nice, ... (core/task) and gets
 * the shell's current directory. Ctrl+C reaches the shell's process:
 * it is passed on to the child, which gets half a second to stop by
 * itself before it is ended.
 *
 * Uses: process_create / process_kill / process_get_by_pid
 *       (process_management), shell_find / shell_run_command (shell),
 *       sys_sleep / sys_sleep_interruptible (process_management),
 *       signal_* (process_management core/signal), cpu_irq_save
 *       (arch/x86/cpu), arg_next / strncpy (lib).
 */

#include "modules/cpu_management/commands/numactl/run_child.h" // this file
#include "modules/process_management/core/scheduler/scheduler.h"    // current process
#include "modules/process_management/core/signal/signal.h"          // signal_*, SIGINT
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep, sys_sleep_interruptible
#include "modules/shell/core/shell/shell.h"                         // shell_find, shell_run_command
#include "modules/shell/core/line/line.h"                           // LINE_MAX
#include "arch/x86/cpu/cpu.h"                                       // cpu_irq_save
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strncpy

#define WAIT_STEP_MS   10       // how often the parent looks whether the child ended
#define GRACE_MS       500      // after Ctrl+C: time to stop by itself before it is killed

// The line the next child runs. The child copies it at once (first
// thing it does), so a command inside it may start another child.
static char next_line[LINE_MAX];

static void child_main(void) {
    char line[LINE_MAX];
    strncpy(line, next_line, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    shell_run_command(line);
}                               // returning ends the process (process_start)

static int alive(int pid) {
    process_t *p = process_get_by_pid(pid);
    return p && p->state != PROCESS_STATE_ZOMBIE;
}

int numactl_run_child(const char *line, numactl_setup_t setup, void *arg) {
    process_t *self = scheduler_get_current_process();
    char name[PROCESS_NAME_LEN];

    arg_next(line, name, sizeof(name));
    if (!name[0] || !shell_find(name))
        return -1;                          // nothing to run
    strncpy(next_line, line, sizeof(next_line) - 1);
    next_line[sizeof(next_line) - 1] = '\0';

    // Interrupts off: the child must not run before it is set up.
    uint32_t irq = cpu_irq_save();
    int pid = process_create(name, child_main, process_caller_uid());
    process_t *child = pid > 0 ? process_get_by_pid(pid) : 0;
    if (child) {
        child->cwd = self ? self->cwd : 0;  // it works where we are
        if (setup && !setup(child, arg)) {
            child->state = PROCESS_STATE_ZOMBIE;    // refused: it never runs
            pid = -1;
        }
    }
    cpu_irq_restore(irq);
    if (!child || pid < 0)
        return -1;

    int interrupted = 0;
    uint32_t grace = 0;

    // Wait until it ended. Ctrl+C reaches us (the shell's process):
    // pass it on, give the child a moment, then stop it.
    while (alive(pid)) {
        if (!sys_sleep_interruptible(WAIT_STEP_MS) && signal_pending(self, SIGINT)) {
            signal_clear(self, SIGINT);
            signal_send(child, SIGINT);
            interrupted = 1;
        }
        if (interrupted && (grace += WAIT_STEP_MS) >= GRACE_MS)
            process_kill(pid);
    }

    // Wait for the reaper too, so the next prompt sees a clean table.
    while (process_get_by_pid(pid))
        sys_sleep(WAIT_STEP_MS);
    if (interrupted)
        signal_send(self, SIGINT);          // so the shell prints ^C
    return pid;
}
