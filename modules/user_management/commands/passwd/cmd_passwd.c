/*
 * cmd_passwd.c - `passwd [user]`: change a password.
 *
 *   passwd          your own: type the old one first, then the new one twice
 *   passwd guest    root only: set guest's password (no old one needed)
 *
 * Uses: users_* (core/users), session_* (core/session), ui_* (ui module).
 */

#include "user_commands.h"
#include "users.h"
#include "session.h"
#include "string.h"
#include "ui.h"

void cmd_passwd(const char *args) {
    const user_t *me = session_user();
    const user_t *target = args[0] ? users_find_name(args) : me;
    char password[SESSION_LINE_MAX + 1];

    if (!me || !target) {
        ui_error("passwd", "no such user");
        return;
    }
    if (target != me && me->uid != UID_ROOT) {
        ui_error("passwd", "only root may change another user's password");
        return;
    }
    if (me->uid != UID_ROOT) {
        int answer = session_ask_password(me);
        if (answer != PASSWORD_RIGHT) {
            if (answer == PASSWORD_WRONG)
                ui_error("passwd", "wrong password");
            return;                         // cancelled: ^C is enough
        }
    }

    int ok = session_ask_new_password(password, sizeof(password)) &&
             users_set_password(target->uid, password);
    memset(password, 0, sizeof(password));
    if (ok)
        ui_ok("passwd", "password changed");
    else
        ui_error("passwd", "not changed: empty, too long, or the two did not match");
}
