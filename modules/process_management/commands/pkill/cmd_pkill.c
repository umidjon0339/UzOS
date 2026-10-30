/*
 * cmd_pkill.c - `pkill [-SIGNAL] [-x] [-u user] pattern`: signal by name.
 *
 *   pkill work            end every process whose name contains "work"
 *   pkill -STOP -x worker freeze every "worker"
 *   pkill -u guest .      end all of guest's processes (root only)
 *
 * The default is SIGKILL, as for UzOS's `kill` (a ring 0 program only
 * looks at other signals when it chooses). Each one goes through the
 * same gates as `kill`: kill_guard / signal_permission. The shell
 * itself is never hit.
 *
 * Uses: process_get_table / process_kill / process_caller_uid (core/process),
 *       scheduler_get_current_process (core/scheduler), signal_send /
 *       signal_number / signal_name (core/signal), def_signal_allowed
 *       (security/signal_abuse/defence/signal_permission), users_find_name
 *       (user_management), arg_next, ui.
 */

#include "modules/process_management/commands/pkill/cmd_pkill.h"     // this file
#include "modules/process_management/core/process/process.h"          // process_get_table, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"            // signal_send, signal_number, signal_name
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed
#include "modules/user_management/core/users/users.h"                 // users_find_name
#include "lib/args/args.h"                                            // arg_next
#include "lib/string/string.h"                                        // strcmp, strncmp, strlen
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

static int contains(const char *name, const char *piece) {
    if (strcmp(piece, ".") == 0)
        return 1;
    int n = strlen(piece);
    for (; *name; name++)
        if (strncmp(name, piece, n) == 0)
            return 1;
    return 0;
}

// Send 'sig' to 'p' through the same gates as `kill`. 1 = sent.
static int send(process_t *p, int sig) {
    if (sig == SIGKILL)
        return process_kill(p->pid);                    // kill_guard decides
    if (!def_signal_allowed(process_caller_uid(), p->uid, p->is_system))
        return 0;
    signal_send(p, sig);
    return 1;
}

void cmd_pkill(const char *args) {
    char word[PROCESS_NAME_LEN], pattern[PROCESS_NAME_LEN] = "";
    int sig = SIGKILL, exact = 0;
    int32_t uid = -1;

    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-x") == 0) {
            exact = 1;
        } else if (strcmp(word, "-u") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            const user_t *u = users_find_name(word);
            if (!u) {
                ui_error("pkill", "no such user");
                return;
            }
            uid = (int32_t)u->uid;
        } else if (word[0] == '-') {
            if (!(sig = signal_number(word + 1))) {
                ui_error("pkill", "unknown signal (see `kill -l`)");
                return;
            }
        } else {
            strncpy(pattern, word, sizeof(pattern) - 1);
        }
    }
    if (!pattern[0]) {
        ui_usage("pkill [-SIGNAL] [-x] [-u user] pattern");
        return;
    }

    process_t *self = scheduler_get_current_process();
    process_t *table = process_get_table();
    int matched = 0, sent = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p == self || p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        if (uid >= 0 && p->uid != (uint32_t)uid)
            continue;
        if (exact ? strcmp(p->name, pattern) != 0 : !contains(p->name, pattern))
            continue;
        matched++;
        sent += send(p, sig);
    }
    ui_start(sent ? UI_OK : UI_ERROR, "pkill");
    ui_printf("SIG%s: %d of %d matching processes%s", signal_name(sig), sent, matched,
              sent < matched ? " (the rest refused: see `audit`)" : "");
    ui_end();
}
