/*
 * cmd_strace.c - `strace`: watch the syscalls of a program.
 *
 *   strace proccalls prctl      run it traced; its children are traced too
 *   [strace] 9 prctl(f, 40000040, 0) = 0
 *   [strace] 9 prctl(f, 40000046, 0) = -1
 *   strace -p 9                 trace a running process from now on
 *   strace -d 9                 stop tracing it
 *
 * The syscall handler prints a line for every int 0x80 of a process
 * whose strace flag is set (process_t.strace, inherited by children).
 * Commands that run inside the kernel call sys_*() directly, without
 * int 0x80: only ring 3 programs (testuser, proccalls, the attacks)
 * show up. Attaching to a running process follows the ptrace rules
 * (ptrace_restrict): yours, dumpable, a descendant - or root.
 *
 * Uses: strace_start_child (start_child.c, this folder), process_get_by_pid
 *       / process_kill (core/process), scheduler_get_current_process,
 *       signal_pending (core/signal), sys_sleep_interruptible,
 *       def_ptrace_attach_ok / def_ptrace_allowed
 *       (security/ptrace_abuse/defence/ptrace_restrict), arg_next, atoi, ui.
 */

#include "modules/process_management/commands/strace/cmd_strace.h"     // this file
#include "modules/process_management/commands/strace/start_child.h"    // strace_start_child
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"             // signal_pending, SIGINT
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep_interruptible
#include "modules/process_management/security/ptrace_abuse/defence/ptrace_restrict/def_ptrace_restrict.h"  // def_ptrace_attach_ok
#include "lib/args/args.h"                                             // arg_next
#include "lib/stdlib/stdlib.h"                                         // atoi
#include "lib/string/string.h"                                         // strcmp
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_usage

#define WAIT_STEP_MS 20

static void trace_on(process_t *child) {
    child->strace = 1;
}

static int alive(uint32_t pid) {
    process_t *p = process_get_by_pid(pid);
    return p && p->state != PROCESS_STATE_ZOMBIE;
}

// `strace -p pid` / `strace -d pid`.
static void attach(const char *pid_text, int on) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid((uint32_t)atoi(pid_text));
    if (!t) {
        ui_error("strace", "no such process");
        return;
    }
    // DEFENCE ptrace_restrict: tracing is debugging.
    int ok = on ? def_ptrace_attach_ok(self, t) : def_ptrace_allowed(self->uid, t->uid, t->is_system);
    if (!ok) {
        ui_error("strace", "refused (see `audit`)");
        return;
    }
    t->strace = on;
    ui_start(UI_OK, "strace");
    ui_printf("%s PID %u (%s)", on ? "tracing" : "stopped tracing", t->pid, t->name);
    ui_end();
}

void cmd_strace(const char *args) {
    char word[12];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0]) {
        ui_usage("strace command [args] | strace -p pid | strace -d pid");
        return;
    }
    if (strcmp(word, "-p") == 0 || strcmp(word, "-d") == 0) {
        int on = word[1] == 'p';
        arg_next(rest, word, sizeof(word));
        attach(word, on);
        return;
    }

    process_t *self = scheduler_get_current_process();
    int pid = strace_start_child(args, trace_on);
    if (pid < 0) {
        ui_error("strace", "unknown command, or no free process slot");
        return;
    }
    while (alive(pid))
        if (!sys_sleep_interruptible(WAIT_STEP_MS) && signal_pending(self, SIGINT)) {
            process_kill(pid);              // Ctrl+C: the shell prints ^C
            return;
        }
}
