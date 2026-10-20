/*
 * cmd_userdel.c - `userdel <name>`: remove an account (root only).
 *
 * Refused for root, for the user you are, and for a user who still has
 * processes running (kill them first). The home directory and files
 * stay; `ls` then shows their owner as "#1000": the uid of a user who no
 * longer exists. That uid is never given to anyone else.
 *
 * Uses: users_find_name / users_delete (core/users), session_user
 *       (core/session), the process table (process_management), ui_*.
 */

#include "user_commands.h"
#include "users.h"
#include "session.h"
#include "process.h"
#include "ui.h"

static int has_processes(uint32_t uid) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].uid == uid)
            return 1;
    return 0;
}

void cmd_userdel(const char *args) {
    const user_t *user = users_find_name(args);
    const user_t *me = session_user();

    if (!args[0]) {
        ui_usage("userdel <name>");
        return;
    }
    if (process_caller_uid() != UID_ROOT) {
        ui_error("userdel", "only root may remove users: type 'su' first");
        return;
    }
    if (!user) {
        ui_error("userdel", "no such user");
        return;
    }
    if (user->uid == UID_ROOT || user == me) {
        ui_error("userdel", "root, and the user you are, cannot be removed");
        return;
    }
    if (has_processes(user->uid)) {
        ui_error("userdel", "that user still has processes: kill them first");
        return;
    }

    uint32_t uid = user->uid;
    users_delete(uid);
    ui_start(UI_OK, "userdel");
    ui_printf("removed %s; files in its home now show owner #%u", args, uid);
    ui_end();
}
