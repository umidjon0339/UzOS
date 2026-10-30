/*
 * cmd_setsid.c - `setsid command [args]`: run it in a session of its own.
 *
 *   setsid spawn        the worker's session is not the shell's
 *   logout              the shell's session hangs up; this one does not
 *
 * A session is a login's group of process groups (process_t.sid). The
 * child becomes the leader of a NEW session and a new process group
 * (sid = pgid = its pid), like setsid() in Unix, so nothing that is sent
 * to the shell's session or group (SIGHUP at logout, kill(-pgid))
 * reaches it, nor anything it starts.
 *
 * Uses: setsid_start_child (start_child.c, this folder), ui.
 */

#include "modules/process_management/commands/setsid/cmd_setsid.h"   // this file
#include "modules/process_management/commands/setsid/start_child.h"  // setsid_start_child
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

static void new_session(process_t *child) {
    child->sid  = child->pid;
    child->pgid = child->pid;
}

void cmd_setsid(const char *args) {
    if (!args[0]) {
        ui_usage("setsid command [args]");
        return;
    }
    int pid = setsid_start_child(args, new_session);
    if (pid < 0) {
        ui_error("setsid", "unknown command, or no free process slot");
        return;
    }
    ui_start(UI_OK, "setsid");
    ui_printf("PID %d runs \"%s\" in its own session %d", pid, args, pid);
    ui_end();
}
