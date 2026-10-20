/*
 * cmd_id.c - `id [user]`: a user's uid, name and home directory.
 *
 *   id          you:   uid=1(guest) home=/home/guest
 *   id root     root:  uid=0(root)  home=/home/root  (the administrator)
 *
 * Uses: users_find_* (core/users), process_caller_uid (process_management),
 *       ui_* (ui module).
 */

#include "user_commands.h"
#include "process.h"
#include "users.h"
#include "ui.h"

void cmd_id(const char *args) {
    const user_t *user = args[0] ? users_find_name(args)
                                 : users_find_uid(process_caller_uid());
    if (!user) {
        ui_error("id", "no such user");
        return;
    }

    ui_printf("uid=%u(%s)  home=%s", user->uid, user->name, user->home);
    ui_color(UI_DIM);
    if (user->uid == UID_ROOT)
        ui_printf("  (the administrator: may do anything)");
    ui_end();
}
