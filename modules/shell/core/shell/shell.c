/*
 * shell.c - Keys in, commands out.
 *
 *   key           what happens
 *   letters       added to the line (line.c); the line is drawn again with
 *                 its command word coloured (highlight.c)
 *   Backspace     last character removed
 *   Tab           completion (complete.c)
 *   Up / Down     history (history.c)
 *   Enter         the line is split into "name args" and the command runs
 *   Ctrl+C        at the prompt: forget the typed line, "^C", a new prompt
 *                 while a command runs: send it SIGINT (it stops early)
 *
 * Uses: the line buffer, history, completion and highlight (core/), vga_* and
 *       KEY_* (device_management), sys_getcwd (filesystem_management)
 *       and session_user (user_management) for the prompt, signal_*
 *       and the current process (process_management) for Ctrl+C, ui_* (ui
 *       module), arg_next (lib/args).
 */

#include "shell.h"
#include "line.h"
#include "history.h"
#include "complete.h"
#include "highlight.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "args.h"
#include "vfs.h"
#include "sys_getcwd.h"
#include "session.h"
#include "scheduler.h"
#include "signal.h"
#include "ui.h"

static shell_command_t commands[SHELL_MAX_COMMANDS];
static int command_count;
static const char *current_group = "Other";
static line_t line;

// The process running a command right now, or NULL while the shell waits
// at the prompt. Read by the keyboard interrupt, hence volatile.
static process_t *volatile running;

/* ---------- Registry ---------- */

void shell_begin_group(const char *title) {
    current_group = title;
}

void shell_register_command(const char *name, const char *description, cmd_func_t handler) {
    if (command_count >= SHELL_MAX_COMMANDS) {
        ui_error("shell", "command table full, a command was not added");
        return;
    }
    commands[command_count++] = (shell_command_t){ name, description, current_group, handler };
}

int shell_command_count(void) {
    return command_count;
}

const shell_command_t *shell_command(int index) {
    return (index >= 0 && index < command_count) ? &commands[index] : 0;
}

const shell_command_t *shell_find(const char *name) {
    for (int i = 0; i < command_count; i++)
        if (strcmp(commands[i].name, name) == 0)
            return &commands[i];
    return 0;
}

/* ---------- Screen ---------- */

// "guest@uzos:~/docs$ " - who, which computer, where; like a Linux shell.
// "~" stands for your home directory, and root's prompt ends in '#'
// instead of '$', so you always see when you have every right.
void shell_print_prompt(void) {
    const user_t *me = session_user();
    char cwd[FS_PATH_MAX];
    const char *where = sys_getcwd(cwd, sizeof(cwd)) == 0 ? cwd : "?";
    int home_len = me ? strlen(me->home) : 0;
    int at_home = me && strncmp(where, me->home, home_len) == 0 &&
                  (where[home_len] == '\0' || where[home_len] == '/');

    if (vga_column() != 0)
        vga_print_str("\n");                    // never glue the prompt to output
    ui_color(me && me->uid == UID_ROOT ? UI_BAD : UI_GOOD);
    vga_print_str(me ? me->name : "?");
    ui_color(UI_LABEL);
    vga_print_str("@");
    ui_color(UI_TITLE);
    vga_print_str("uzos");
    ui_color(UI_LABEL);
    vga_print_str(":");
    ui_color(UI_LINK);
    if (at_home) {
        vga_print_str("~");
        where += home_len;
    }
    vga_print_str(where);
    ui_color(UI_LABEL);
    vga_print_str(me && me->uid == UID_ROOT ? "# " : "$ ");
    ui_reset();
}

// Draw the typed line again, in place and coloured. 'shown' is how many
// characters of the line are on the screen right now (before the change):
// that many backspaces take us back to where the line starts.
static void redraw(int shown) {
    for (int i = 0; i < shown; i++)
        vga_put_char('\b');
    highlight_print(line.text);
}

static void show_line(const char *text) {
    int shown = line.length;
    line_set(&line, text);
    redraw(shown);
}

/* ---------- Running a command ---------- */

int shell_run_command(const char *input) {
    char name[32];
    char args[LINE_MAX];

    // "  cat   notes  " -> name "cat", args "notes"
    strcpy(args, arg_next(input, name, sizeof(name)));
    int end = strlen(args);
    while (end > 0 && args[end - 1] == ' ')
        args[--end] = '\0';

    if (!name[0])
        return 1;                               // an empty line: nothing to do

    const shell_command_t *cmd = shell_find(name);
    if (!cmd) {
        ui_start(UI_ERROR, name);
        vga_print_str("command not found (type 'help' for the list)");
        ui_end();
        return 0;
    }
    cmd->handler(args);
    ui_reset();                                 // a command may leave any colour
    return 1;
}

static void run(const char *input) {
    // While the command runs, Ctrl+C becomes SIGINT for this process.
    process_t *self = scheduler_get_current_process();
    signal_clear(self, SIGINT);                 // nothing old may stop it
    running = self;
    shell_run_command(input);
    running = 0;

    if (signal_pending(self, SIGINT)) {         // it was interrupted
        signal_clear(self, SIGINT);
        vga_print_str("^C\n");
    }
}

// Called by the keyboard interrupt on Ctrl+C. 1 = handled here; 0 = let
// it reach the queue, so the prompt can clear the line (on_ctrl_c below).
static int ctrl_c_pressed(void) {
    process_t *job = running;
    if (!job)
        return 0;
    signal_send(job, SIGINT);
    return 1;
}

/* ---------- Keys ---------- */

static void on_enter(void) {
    char input[LINE_MAX];

    vga_put_char('\n');
    strcpy(input, line.text);       // a copy: the command may outlive the line
    history_add(input);
    history_stop_browsing();
    line_clear(&line);

    run(input);
    shell_print_prompt();
}

// Ctrl+C at the prompt: give up the typed line, like bash.
static void on_ctrl_c(void) {
    vga_print_str("^C\n");
    history_stop_browsing();
    line_clear(&line);
    shell_print_prompt();
}

static void on_tab(void) {
    int before = line.length;
    complete_result_t result = complete_line(&line);

    if (result == COMPLETE_ADDED) {
        redraw(before);                         // the new part, and maybe a new colour
    } else if (result == COMPLETE_LISTED) {
        shell_print_prompt();
        highlight_print(line.text);
    }
}

void shell_handle_key(char c) {
    unsigned char key = (unsigned char)c;

    if (c == '\n') {
        on_enter();
    } else if (c == '\b') {
        int shown = line.length;
        if (line_backspace(&line))
            redraw(shown);
    } else if (c == '\t') {
        on_tab();
    } else if (key == KEY_CTRL_C) {
        on_ctrl_c();
    } else if (key == KEY_UP) {
        const char *older = history_older();
        if (older)
            show_line(older);
    } else if (key == KEY_DOWN && history_browsing()) {
        const char *newer = history_newer();
        show_line(newer ? newer : "");          // past the newest: empty line
    } else if (key >= ' ' && key < 0x7F) {      // printable: type it
        int shown = line.length;
        if (line_insert(&line, c))
            redraw(shown);
    }
    // Everything else (Escape, arrows left/right, ...) is ignored.
}

void shell_init(void) {
    command_count = 0;
    current_group = "Other";
    line_clear(&line);
    keyboard_on_ctrl_c(ctrl_c_pressed);
}
