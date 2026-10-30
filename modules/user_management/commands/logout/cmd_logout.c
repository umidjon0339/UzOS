/*
 * cmd_logout.c - `logout`: end the session, back to the login prompt.
 *
 * Like a terminal that closes, logout "hangs up" the session: every job
 * the shell started gets SIGHUP and ends, unless it was started with
 * `nohup` (or `setsid`, which puts it in a session of its own).
 *
 * Uses: session_login_prompt (core/session), vga_clear (device_management),
 *       process_hangup (process_management core/process), ui_* (ui module).
 */

#include "user_commands.h"
#include "session.h"
#include "vga.h"
#include "ui.h"
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/process/process.h"      // process_hangup

void cmd_logout(const char *args) {
    (void)args;
    const user_t *me = session_user();
    process_t *self = scheduler_get_current_process();

    // The terminal session ends: its jobs get SIGHUP (nohup'd ones live on).
    int ended = process_hangup(self->sid, self);

    ui_start(UI_INFO, "logout");
    ui_printf("goodbye, %s", me ? me->name : "?");
    if (ended)
        ui_printf(" (%d job%s ended by SIGHUP)", ended, ended == 1 ? "" : "s");
    ui_end();
    session_login_prompt();             // returns when someone has logged in
}
