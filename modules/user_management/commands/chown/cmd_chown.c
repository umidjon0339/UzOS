/*
 * cmd_chown.c - `chown <user> <path>`: give a file to another user.
 *
 *   chown guest /home/guest/notes
 *
 * Needs CAP_CHOWN (root). (If anyone could give files away, a user could hand a
 * file to someone else and make them look responsible for it, or slip
 * past that user's limits. Linux has the same rule.)
 *
 * Uses: users_find_name (core/users), caps_require (security_management),
 *       vfs_lookup (filesystem_management), arg_next (lib), ui_* (ui module).
 */

#include "user_commands.h"
#include "users.h"
#include "caps.h"
#include "vfs.h"
#include "args.h"
#include "ui.h"

void cmd_chown(const char *args) {
    char name[USER_NAME_MAX + 1];
    const char *path = arg_next(args, name, sizeof(name));

    if (!name[0] || !path[0]) {
        ui_usage("chown <user> <path>");
        return;
    }
    if (!caps_require(CAP_CHOWN, "chown"))
        return;

    const user_t *user = users_find_name(name);
    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    if (!user || !node) {
        ui_error("chown", user ? "no such file or directory" : "no such user");
        return;
    }

    node->owner = user->uid;
    ui_start(UI_OK, "chown");
    ui_printf("%s now belongs to %s", path, user->name);
    ui_end();
}
