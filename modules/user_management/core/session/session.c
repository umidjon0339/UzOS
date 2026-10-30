/*
 * session.c - The login prompt and switching users.
 *
 * While the login prompt (or a password question) waits, the shell is
 * not reading keys itself, so we read them here: take a key from the
 * keyboard queue, or sleep one tick if there is none (sleeping, not
 * spinning, so the CPU stays free).
 *
 * Ctrl+C cancels the line, like Esc. It arrives one of two ways: as the
 * key KEY_CTRL_C (at boot, when no command runs), or as SIGINT to the
 * shell (inside `su`, `passwd`, `logout`, ...). Either way the login
 * prompt simply asks again: Ctrl+C never gets you past it.
 *
 * Uses: users_* (core/users), the shell process's uid and cwd
 *       (process_management), vfs_lookup (filesystem_management),
 *       keyboard_pop_key / KEY_ESCAPE / KEY_CTRL_C (device_management),
 *       sys_sleep / sys_sleep_interruptible and signal_* (process_management),
 *       ui_* (ui module), string (lib).
 */

#include "session.h"
#include "modules/network_management/core/hostname/hostname.h"   // hostname_get
#include "scheduler.h"
#include "vfs.h"
#include "keyboard.h"
#include "sys_sleep.h"
#include "signal.h"
#include "string.h"
#include "ui.h"

#define KEY_POLL_MS   10
#define WRONG_WAIT_MS 1000      // a short pause after a wrong password

static process_t *shell;        // the process the user types into

void session_init(void) {
    shell = scheduler_get_current_process();
}

const user_t *session_user(void) {
    return shell ? users_find_uid(shell->uid) : 0;
}

void session_switch(const user_t *user) {
    if (!shell || !user)
        return;
    shell->uid = user->uid;

    vfs_node_t *home = vfs_lookup(fs_root, user->home);
    shell->cwd = home ? home : fs_root;
}

/* ---------- Reading what the user types ---------- */

// Ctrl+C: show it, forget the line.
static int cancelled(char *out) {
    signal_clear(shell, SIGINT);        // dealt with here, not by the shell
    vga_print_str("^C\n");
    out[0] = '\0';
    return 0;
}

int session_read_line(char *out, int max, int hidden) {
    int length = 0;

    while (1) {
        char c = keyboard_pop_key();
        if (!c) {
            if (!sys_sleep_interruptible(KEY_POLL_MS))
                return cancelled(out);  // SIGINT: Ctrl+C while a command runs
            continue;
        }

        if (c == KEY_CTRL_C)
            return cancelled(out);      // Ctrl+C at boot, as a key

        if (c == '\n') {
            vga_put_char('\n');
            out[length] = '\0';
            return 1;
        }
        if (c == KEY_ESCAPE) {
            vga_put_char('\n');
            out[0] = '\0';
            return 0;
        }
        if (c == '\b') {
            if (length > 0) {
                length--;
                if (!hidden)
                    vga_put_char('\b');
            }
            continue;
        }
        if ((unsigned char)c >= ' ' && (unsigned char)c < 0x7F && length < max - 1) {
            out[length++] = c;
            if (!hidden)
                vga_put_char(c);
        }
    }
}

int session_ask_password(const user_t *user) {
    char password[SESSION_LINE_MAX + 1];

    ui_color(UI_LABEL);
    vga_print_str("Password: ");
    ui_reset();
    int typed = session_read_line(password, sizeof(password), 1);
    int right = typed && users_check_password(user, password);
    memset(password, 0, sizeof(password));      // do not leave it on the stack

    if (!typed)
        return PASSWORD_CANCELLED;
    return right ? PASSWORD_RIGHT : PASSWORD_WRONG;
}

int session_ask_new_password(char *out, int max) {
    char again[SESSION_LINE_MAX + 1];

    vga_print_str("New password: ");
    if (!session_read_line(out, max, 1) || !out[0]) {
        memset(out, 0, max);                    // cancelled halfway: wipe it
        return 0;
    }
    vga_print_str("Again: ");
    int typed = session_read_line(again, sizeof(again), 1);

    int same = typed && strcmp(out, again) == 0;
    memset(again, 0, sizeof(again));
    if (!same)
        memset(out, 0, max);                    // not used: wipe it
    return same;
}

/* ---------- The login prompt ---------- */

void session_login_prompt(void) {
    char name[SESSION_LINE_MAX + 1];

    ui_hint("Accounts: root (password uzos), guest (password guest).");
    while (1) {
        vga_print_str("\n");
        ui_color(UI_TITLE);
        vga_print_str(hostname_get());
        ui_color(UI_LABEL);
        vga_print_str(" login: ");
        ui_reset();
        if (!session_read_line(name, sizeof(name), 0) || !name[0])
            continue;

        // Ask for a password even when the name is unknown, and give the
        // same answer: the prompt does not tell which names exist.
        const user_t *user = users_find_name(name);
        int answer = session_ask_password(user);
        if (answer == PASSWORD_CANCELLED)
            continue;                   // Ctrl+C / Esc: just ask again
        if (answer == PASSWORD_RIGHT && user) {
            session_switch(user);
            ui_start(UI_OK, 0);
            ui_printf("Welcome, %s. Your home is %s", user->name, user->home);
            ui_end();
            return;
        }
        ui_error("login", "wrong name or password");
        sys_sleep(WRONG_WAIT_MS);
    }
}
