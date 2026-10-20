/*
 * cmd_users.c - `users`: every account in the user table.
 *
 *   ┌─ Users ──────────────────────────────────────────────┐
 *   │   UID  NAME         HOME           PROCS  HASH        │
 *   │     0  root         /home/root         3  4f1c09a2... │
 *
 * HASH is the start of the stored password hash. The passwords
 * themselves are not stored anywhere, so they cannot be shown.
 *
 * Uses: users_slot (core/users), the process table (process_management),
 *       box_* / ui_* (ui module).
 */

#include "user_commands.h"
#include "users.h"
#include "process.h"
#include "ui.h"
#include "box.h"

#define WIDTH       62
#define HASH_SHOWN  4               // bytes of the hash to print

static int processes_of(uint32_t uid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].uid == uid)
            n++;
    return n;
}

void cmd_users(const char *args) {
    (void)args;

    box_begin("Users", WIDTH);
    box_row();
    ui_color(UI_TITLE);
    vga_print_str("  UID  NAME         HOME             PROCS  HASH");
    box_row_end();

    for (int i = 0; i < USERS_MAX; i++) {
        const user_t *user = users_slot(i);
        if (!user)
            continue;

        box_row();
        ui_printf("%5u  ", user->uid);
        ui_color(user->uid == UID_ROOT ? UI_BAD : UI_GOOD);
        ui_cell(user->name, 12);
        ui_reset();
        ui_cell(user->home, 16);
        ui_printf("%6d  ", processes_of(user->uid));
        ui_color(UI_DIM);
        for (int b = 0; b < HASH_SHOWN; b++)
            ui_printf("%02x", user->hash[b]);
        vga_print_str("...");
        box_row_end();
    }
    box_end();
    ui_hint("passwords are kept only as salted SHA-256 hashes");
}
