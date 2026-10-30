/*
 * start_child.c - Start a command line as a child; do not wait for it.
 *
 *   shell                                child
 *   timeout_start_child("sleep 30")
 *     process_create  ------------>      (not running yet: interrupts off)
 *     setup(child)
 *     interrupts on, wait until  ---->   copies the line, then runs it
 *     the child has the line             through the shell's table
 *   back to the prompt                   ...ends; the reaper frees it
 *
 * Uses: process_create / process_get_by_pid / process_caller_uid
 *       (core/process), scheduler_get_current_process (core/scheduler),
 *       shell_find / shell_run_command (shell), sys_sleep, cpu_irq_save,
 *       arg_next / strncpy (lib).
 */

#include "modules/process_management/commands/timeout/start_child.h"    // this file
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/syscalls/sleep/sys_sleep.h"      // sys_sleep
#include "modules/shell/core/shell/shell.h"                           // shell_find, shell_run_command
#include "modules/shell/core/line/line.h"                             // LINE_MAX
#include "arch/x86/cpu/cpu.h"                                         // cpu_irq_save, cpu_irq_restore
#include "lib/args/args.h"                                            // arg_next
#include "lib/string/string.h"                                        // strncpy

#define COPY_WAIT_MS 5
#define COPY_TRIES   40

// The line the next child runs; it copies it first thing.
static char next_line[LINE_MAX];
static volatile int copied;

static void child_main(void) {
    char line[LINE_MAX];
    strncpy(line, next_line, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    copied = 1;                         // next_line is free again
    shell_run_command(line);
}                                       // returning ends the process

int timeout_start_child(const char *line, void (*setup)(process_t *child)) {
    process_t *self = scheduler_get_current_process();
    char name[PROCESS_NAME_LEN];

    arg_next(line, name, sizeof(name));
    if (!name[0] || !shell_find(name))
        return -1;
    strncpy(next_line, line, sizeof(next_line) - 1);
    next_line[sizeof(next_line) - 1] = '\0';
    copied = 0;

    uint32_t irq = cpu_irq_save();      // it must not run before it is set up
    int pid = process_create(name, child_main, process_caller_uid());
    process_t *child = pid > 0 ? process_get_by_pid(pid) : 0;
    if (child) {
        child->cwd = self ? self->cwd : 0;
        if (setup)
            setup(child);
    }
    cpu_irq_restore(irq);
    if (!child)
        return -1;

    for (int i = 0; i < COPY_TRIES && !copied; i++)
        sys_sleep(COPY_WAIT_MS);
    return pid;
}
