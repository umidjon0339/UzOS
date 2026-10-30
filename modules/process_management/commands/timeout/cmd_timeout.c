/*
 * cmd_timeout.c - `timeout [-s SIG] seconds command [args]`: a time limit.
 *
 *   timeout 2 sleep 10       sleep ends after 2 s: "timed out"
 *   timeout -s INT 2 top     top gets SIGINT after 2 s (it stops itself)
 *
 * The command runs in a child (start_child.c); the shell waits. When the
 * time is up the child gets the signal (default SIGTERM); if it still
 * runs one second later it is killed (SIGKILL), since a ring 0 command
 * only looks at the signals it wants. Ctrl+C ends it at once.
 *
 * Uses: timeout_start_child (start_child.c, this folder), process_get_by_pid
 *       / process_kill (core/process), scheduler_get_current_process,
 *       signal_send / signal_number / signal_pending / signal_name
 *       (core/signal), sys_sleep_interruptible, timer_ticks / TIMER_HZ
 *       (device_management), arg_next, atoi, ui.
 */

#include "modules/process_management/commands/timeout/cmd_timeout.h"   // this file
#include "modules/process_management/commands/timeout/start_child.h"   // timeout_start_child
#include "modules/process_management/core/scheduler/scheduler.h"        // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"              // signal_send, signal_number, SIGTERM
#include "modules/process_management/syscalls/sleep/sys_sleep.h"        // sys_sleep_interruptible
#include "modules/device_management/timer/timer.h"                      // timer_ticks, TIMER_HZ
#include "lib/args/args.h"                                              // arg_next
#include "lib/stdlib/stdlib.h"                                          // atoi
#include "lib/string/string.h"                                          // strcmp
#include "modules/ui/core/style/ui.h"                                   // ui_printf, ui_usage

#define WAIT_STEP_MS 20
#define KILL_AFTER_S 1          // after the signal, this long until SIGKILL

static int alive(uint32_t pid) {
    process_t *p = process_get_by_pid(pid);
    return p && p->state != PROCESS_STATE_ZOMBIE;
}

void cmd_timeout(const char *args) {
    char word[16];
    int sig = SIGTERM;
    const char *rest = arg_next(args, word, sizeof(word));
    if (strcmp(word, "-s") == 0) {
        rest = arg_next(rest, word, sizeof(word));
        if (!(sig = signal_number(word))) {
            ui_error("timeout", "unknown signal (see `kill -l`)");
            return;
        }
        rest = arg_next(rest, word, sizeof(word));
    }
    int seconds = atoi(word);
    while (*rest == ' ')
        rest++;
    if (seconds <= 0 || !*rest) {
        ui_usage("timeout [-s SIG] seconds command [args]");
        return;
    }

    process_t *self = scheduler_get_current_process();
    int pid = timeout_start_child(rest, 0);
    if (pid < 0) {
        ui_error("timeout", "unknown command, or no free process slot");
        return;
    }

    uint32_t deadline = timer_ticks + (uint32_t)seconds * TIMER_HZ;
    uint32_t kill_at = 0;
    while (alive(pid)) {
        if (!sys_sleep_interruptible(WAIT_STEP_MS) && signal_pending(self, SIGINT)) {
            process_kill(pid);                  // Ctrl+C: the shell prints ^C
            return;
        }
        if (!kill_at && (int32_t)(timer_ticks - deadline) >= 0) {
            signal_send(process_get_by_pid(pid), sig);
            kill_at = timer_ticks + KILL_AFTER_S * TIMER_HZ;
            ui_start(UI_WARN, "timeout");
            ui_printf("%d s are up: SIG%s to PID %d", seconds, signal_name(sig), pid);
            ui_end();
        }
        if (kill_at && alive(pid) && (int32_t)(timer_ticks - kill_at) >= 0) {
            process_kill(pid);
            ui_start(UI_WARN, "timeout");
            ui_printf("PID %d still ran: SIGKILL", pid);
            ui_end();
        }
    }
    if (!kill_at) {
        ui_start(UI_OK, "timeout");
        ui_printf("\"%s\" finished in time", rest);
        ui_end();
    }
}
