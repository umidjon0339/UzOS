/*
 * cmd_logout.c - `logout`: end the session, back to the login prompt.
 *
 * Uses: session_login_prompt (core/session), vga_clear (device_management),
 *       ui_* (ui module).
 */

#include "user_commands.h"
#include "session.h"
#include "vga.h"
#include "ui.h"

void cmd_logout(const char *args) {
    (void)args;
    const user_t *me = session_user();

    ui_start(UI_INFO, "logout");
    ui_printf("goodbye, %s", me ? me->name : "?");
    ui_end();
    session_login_prompt();             // returns when someone has logged in
}
