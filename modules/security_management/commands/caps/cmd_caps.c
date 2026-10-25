/*
 * cmd_caps.c - `caps [user]`: which privileges a user holds.
 *
 *   caps          you
 *   caps root     root
 *
 *   > Capabilities of guest (uid 1)
 *     x CAP_KILL       stop processes of other users
 *     x CAP_SETUID     become another user, no password
 *     ...
 *
 * Every privileged check in the kernel asks for one of these (core/caps).
 *
 * Uses: caps_* (core/caps), users_find_* (user_management),
 *       process_caller_uid (process_management), ui_* / ui_cell (ui).
 */

#include "security_commands.h"
#include "caps.h"
#include "users.h"
#include "process.h"
#include "format.h"
#include "ui.h"
#include "box.h"

void cmd_caps(const char *args) {
    const user_t *user = args[0] ? users_find_name(args)
                                 : users_find_uid(process_caller_uid());
    if (!user) {
        ui_error("caps", "no such user");
        return;
    }

    char title[48];
    format(title, sizeof(title), "Capabilities of %s (uid %u)", user->name, user->uid);
    ui_title(title);

    int held = 0;
    for (int cap = 0; cap < CAP_COUNT; cap++) {
        int has = caps_has(user->uid, (cap_t)cap);
        held += has;

        ui_color(has ? UI_GOOD : UI_DIM);
        vga_print_str(has ? "  " GLYPH_CHECK " " : "  " GLYPH_CROSS " ");
        ui_color(has ? VGA_COLOR_WHITE : UI_DIM);
        ui_cell(caps_name((cap_t)cap), 15);
        vga_print_str(caps_about((cap_t)cap));
        ui_end();
    }

    ui_color(UI_LABEL);
    ui_printf("  %s holds %d of %d", user->name, held, CAP_COUNT);
    ui_end();
    ui_hint("  root holds all of them, other users none (the table in core/caps)");
}
