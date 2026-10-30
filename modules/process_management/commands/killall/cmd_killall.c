/*
 * cmd_killall.c - `killall [-SIGNAL] name`: signal every process called
 * exactly 'name'.
 *
 *   killall worker           end every "worker"
 *   killall -CONT worker     let every stopped "worker" carry on
 *
 * Like `pkill -x name`. The default is SIGKILL, as for UzOS's `kill`;
 * the gates are the same (kill_guard / signal_permission), and the shell
 * itself is never hit.
 *
 * Uses: process_get_table / process_kill / process_caller_uid (core/process),
 *       scheduler_get_current_process (core/scheduler), signal_send /
 *       signal_number / signal_name (core/signal), def_signal_allowed
 *       (security/signal_abuse/defence/signal_permission), arg_next, ui.
 */

#include "modules/process_management/commands/killall/cmd_killall.h" // this file
#include "modules/process_management/core/process/process.h"          // process_get_table, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"            // signal_send, signal_number, signal_name
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed
#include "lib/args/args.h"                                            // arg_next
#include "lib/string/string.h"                                        // strcmp
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

// Send 'sig' to 'p' through the same gates as `kill`. 1 = sent.
static int send(process_t *p, int sig) {
    if (sig == SIGKILL)
        return process_kill(p->pid);
    if (!def_signal_allowed(process_caller_uid(), p->uid, p->is_system))
        return 0;
    signal_send(p, sig);
    return 1;
}

void cmd_killall(const char *args) {
    char word[PROCESS_NAME_LEN];
    int sig = SIGKILL;
    const char *rest = arg_next(args, word, sizeof(word));

    if (word[0] == '-') {
        if (!(sig = signal_number(word + 1))) {
            ui_error("killall", "unknown signal (see `kill -l`)");
            return;
        }
        rest = arg_next(rest, word, sizeof(word));
    }
    if (!word[0]) {
        ui_usage("killall [-SIGNAL] name");
        return;
    }

    process_t *self = scheduler_get_current_process();
    process_t *table = process_get_table();
    int matched = 0, sent = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p == self || p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE ||
            strcmp(p->name, word) != 0)
            continue;
        matched++;
        sent += send(p, sig);
    }
    if (!matched) {
        ui_error("killall", "no process found");
        return;
    }
    ui_start(sent ? UI_OK : UI_ERROR, "killall");
    ui_printf("SIG%s: %d of %d \"%s\"%s", signal_name(sig), sent, matched, word,
              sent < matched ? " (the rest refused: see `audit`)" : "");
    ui_end();
}
