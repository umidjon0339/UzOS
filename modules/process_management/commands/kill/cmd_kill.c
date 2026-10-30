/*
 * cmd_kill.c - `kill [-SIGNAL] <pid>...`: stop a process, or send it a signal.
 *
 *   kill 7               end PID 7 at once (SIGKILL)
 *   kill -TERM 7         send SIGTERM (also -15, -SIGTERM, -s TERM)
 *   kill -STOP 7         freeze it; `kill -CONT 7` lets it carry on
 *   kill -l              list every signal; `kill -l 15` names one
 *
 * Without a signal UzOS's kill sends SIGKILL, not SIGTERM as Linux does:
 * a ring 0 program (spawn, exec) only looks at its signals when it
 * chooses, so SIGTERM might never stop it. A ring 3 program gets any
 * signal delivered on its next return to ring 3: its handler runs, or
 * the default action (most of them end it).
 *
 * Uses: process_kill / process_get_by_pid (core/process), signal_send /
 *       signal_number / signal_name / signal_default_action (core/signal),
 *       def_signal_allowed (security/signal_abuse/defence/signal_permission),
 *       arg_next (lib/args), atoi (lib/stdlib), ui_* (ui module).
 * Why the command checks almost nothing itself: the real rules live in
 * kill_guard (inside process_kill) and signal_permission, so a syscall
 * or another module cannot skip them.
 */

#include "modules/process_management/commands/kill/cmd_kill.h"                                          // this file
#include "modules/process_management/core/process/process.h"                                            // process_kill, process_get_by_pid
#include "modules/process_management/core/signal/signal.h"                                              // signal_send, signal_number, signal_name
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed
#include "lib/args/args.h"                                                                              // arg_next
#include "lib/stdlib/stdlib.h"                                                                          // atoi
#include "modules/ui/core/style/ui.h"                                                                   // ui_usage, ui_error, ui_ok

static const char *const action_words[] = {
    [ACT_TERM] = "end", [ACT_CORE] = "end+core", [ACT_IGNORE] = "ignore",
    [ACT_STOP] = "stop", [ACT_CONT] = "continue",
};

// `kill -l`: every signal, three to a line, with its default action.
static void list_signals(void) {
    for (int sig = 1; sig < SIGNAL_COUNT; sig++) {
        ui_printf("%2d) SIG%-7s %-9s", sig, signal_name(sig),
                  action_words[signal_default_action(sig)]);
        vga_print_str(sig % 3 == 0 ? "\n" : "  ");
    }
    vga_print_str("\n");
}

// Send 'sig' to 'pid' and say what happened.
static void send(int pid, int sig) {
    if (sig == SIGKILL) {
        if (process_kill(pid))
            ui_ok("kill", "process ended");
        else
            ui_error("kill", "refused: no such process, or a system one (see `audit`)");
        return;
    }

    process_t *t = process_get_by_pid(pid);
    if (!t) {
        ui_error("kill", "no such process");
        return;
    }
    if (!def_signal_allowed(process_caller_uid(), t->uid, t->is_system)) {
        ui_error("kill", "refused: not your process, or a system one (see `audit`)");
        return;
    }
    signal_send(t, sig);
    ui_start(UI_OK, "kill");
    ui_printf("SIG%s sent to PID %d", signal_name(sig), pid);
    ui_end();
}

void cmd_kill(const char *args) {
    char word[16];
    const char *rest = arg_next(args, word, sizeof(word));
    int sig = SIGKILL;

    if (word[0] == '\0') {
        ui_usage("kill [-SIGNAL | -s SIGNAL] <pid>...  |  kill -l [N]");
        return;
    }

    if (word[0] == '-' && word[1] == 'l' && word[2] == '\0') {     // kill -l [N]
        arg_next(rest, word, sizeof(word));
        if (word[0])
            ui_printf("%s\n", signal_name(atoi(word)));
        else
            list_signals();
        return;
    }

    if (word[0] == '-') {                       // -TERM, -15, -SIGTERM, -s TERM
        const char *name = word + 1;
        if (word[1] == 's' && word[2] == '\0') {
            rest = arg_next(rest, word, sizeof(word));
            name = word;
        }
        sig = signal_number(name);
        if (!sig) {
            ui_error("kill", "unknown signal (see `kill -l`)");
            return;
        }
        rest = arg_next(rest, word, sizeof(word));
    }

    if (word[0] == '\0') {
        ui_usage("kill [-SIGNAL] <pid>...");
        return;
    }
    for (; word[0]; rest = arg_next(rest, word, sizeof(word))) {
        int pid = atoi(word);
        if (pid <= 0)
            ui_error("kill", "not a valid PID");
        else
            send(pid, sig);
    }
}
