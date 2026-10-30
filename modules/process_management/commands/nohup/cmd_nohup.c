/*
 * cmd_nohup.c - `nohup command [args]`: run it so that logout does not end it.
 *
 *   nohup sleep 300     [1] sleep (PID 6), ignoring SIGHUP
 *   logout              every other job ends with SIGHUP; this one lives
 *
 * When a terminal session ends, its jobs get SIGHUP ("hang up"), whose
 * default action ends them (process_hangup, called by logout). nohup
 * starts the command in a background child that IGNORES SIGHUP. An
 * ignored signal stays ignored in everything that child starts, so the
 * whole command is safe.
 *
 * Uses: nohup_start_child (start_child.c, this folder), SIGHUP / SIG_IGN
 *       (core/signal), ui.
 */

#include "modules/process_management/commands/nohup/cmd_nohup.h"     // this file
#include "modules/process_management/commands/nohup/start_child.h"   // nohup_start_child
#include "modules/process_management/core/signal/signal.h"            // SIGHUP, SIG_IGN
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

static void ignore_hangup(process_t *child) {
    child->sig_handler[SIGHUP] = SIG_IGN;
}

void cmd_nohup(const char *args) {
    if (!args[0]) {
        ui_usage("nohup command [args]");
        return;
    }
    int pid = nohup_start_child(args, ignore_hangup);
    if (pid < 0) {
        ui_error("nohup", "unknown command, or no free process slot");
        return;
    }
    ui_start(UI_OK, "nohup");
    ui_printf("PID %d runs \"%s\" in the background, ignoring SIGHUP", pid, args);
    ui_end();
}
