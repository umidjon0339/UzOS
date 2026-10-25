/*
 * cmd_useradd.c - `useradd <name>`: create a new account (CAP_USERS: root).
 *
 *   useradd alice      asks for alice's password twice, then:
 *                      uid 1000, home /home/alice (owned by alice)
 *
 * Names are 1-15 characters: "a"-"z", "0"-"9", "_", starting with a
 * letter. Each new user gets the next uid; uids are never reused.
 *
 * Uses: users_add / users_name_ok (core/users), session_ask_new_password
 *       (core/session), caps_require (security_management), ui_*.
 */

#include "user_commands.h"
#include "users.h"
#include "session.h"
#include "caps.h"
#include "string.h"
#include "ui.h"

void cmd_useradd(const char *args) {
    char password[SESSION_LINE_MAX + 1];

    if (!args[0]) {
        ui_usage("useradd <name>");
        return;
    }
    if (!caps_require(CAP_USERS, "useradd"))
        return;
    if (!users_name_ok(args) || users_find_name(args)) {
        ui_error("useradd", "name taken, or not 1-15 of a-z 0-9 _ (starting with a letter)");
        return;
    }
    if (!session_ask_new_password(password, sizeof(password))) {   // wipes it on failure
        ui_error("useradd", "not added: empty password, or the two did not match");
        return;
    }

    int uid = users_add(args, password);
    memset(password, 0, sizeof(password));
    if (uid < 0) {
        ui_error("useradd", "the user table is full");
        return;
    }
    ui_start(UI_OK, "useradd");
    ui_printf("%s added: uid %d, home /home/%s", args, uid, args);
    ui_end();
}
