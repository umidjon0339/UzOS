/*
 * cmd_su.c - `su [user]`: become another user ("switch user").
 *
 *   su          become root
 *   su guest    become guest
 *
 * You must type that user's password, except when you hold CAP_SETUID
 * (root): root may become anyone. Afterwards the shell runs with the new uid and
 * starts in the new user's home. `logout` ends the session.
 *
 * Uses: users_find_name (core/users), session_* (core/session),
 *       caps_caller_has (security_management), ui_* (ui module).
 */

#include "user_commands.h"
#include "users.h"
#include "session.h"
#include "caps.h"
#include "ui.h"

void cmd_su(const char *args) {
    const user_t *target = users_find_name(args[0] ? args : "root");

    if (!target) {
        ui_error("su", "no such user");
        return;
    }
    if (!caps_caller_has(CAP_SETUID)) {
        int answer = session_ask_password(target);
        if (answer != PASSWORD_RIGHT) {
            if (answer == PASSWORD_WRONG)
                ui_error("su", "wrong password");
            return;                         // cancelled: ^C is enough
        }
    }

    session_switch(target);
    ui_start(UI_OK, "su");
    ui_printf("you are now %s (uid %u)", target->name, target->uid);
    ui_end();
}
