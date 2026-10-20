/*
 * shell.c - Keys in, commands out.
 *
 *   key           what happens
 *   letters       added to the line (line.c) and shown
 *   Backspace     last character removed
 *   Tab           completion (complete.c)
 *   Up / Down     history (history.c)
 *   Enter         the line is split into "name args" and the command runs
 *
 * Uses: the line buffer, history and completion (core/), vga_* and
 *       KEY_* (device_management), sys_getcwd (filesystem_management)
 *       for the prompt, ui_* (ui module), arg_next (lib/args).
 */

#include "shell.h"
#include "line.h"
#include "history.h"
#include "complete.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "args.h"
#include "vfs.h"
#include "sys_getcwd.h"
#include "ui.h"

static shell_command_t commands[SHELL_MAX_COMMANDS];
static int command_count;
static const char *current_group = "Other";
static line_t line;

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

// "root@uzos:/docs$ " - who, which computer, where; like a Linux shell.
// (UzOS has no users yet: the shell always runs as root, UID 0.)
void shell_print_prompt(void) {
    char cwd[FS_PATH_MAX];
    if (vga_column() != 0)
        vga_print_str("\n");                    // never glue the prompt to output
    ui_color(UI_GOOD);
    vga_print_str("root");
    ui_color(UI_LABEL);
    vga_print_str("@");
    ui_color(UI_TITLE);
    vga_print_str("uzos");
    ui_color(UI_LABEL);
    vga_print_str(":");
    ui_color(UI_LINK);
    vga_print_str(sys_getcwd(cwd, sizeof(cwd)) == 0 ? cwd : "?");
    ui_color(UI_LABEL);
    vga_print_str("$ ");
    ui_reset();
}

// Take the typed text off the screen (one backspace per character).
static void erase_typed(void) {
    for (int i = 0; i < line.length; i++)
        vga_put_char('\b');
}

static void show_line(const char *text) {
    erase_typed();
    line_set(&line, text);
    vga_print_str(line.text);
}

/* ---------- Running a command ---------- */

static void run(const char *input) {
    char name[32];
    char args[LINE_MAX];

    // "  cat   notes  " -> name "cat", args "notes"
    strcpy(args, arg_next(input, name, sizeof(name)));
    int end = strlen(args);
    while (end > 0 && args[end - 1] == ' ')
        args[--end] = '\0';

    if (!name[0])
        return;                                 // an empty line

    const shell_command_t *cmd = shell_find(name);
    if (!cmd) {
        ui_start(UI_ERROR, name);
        vga_print_str("command not found (type 'help' for the list)");
        ui_end();
        return;
    }
    cmd->handler(args);
    ui_reset();                                 // a command may leave any colour
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

static void on_tab(void) {
    int before = line.length;
    complete_result_t result = complete_line(&line);

    if (result == COMPLETE_ADDED) {
        vga_print_str(&line.text[before]);      // show only the new part
    } else if (result == COMPLETE_LISTED) {
        shell_print_prompt();
        vga_print_str(line.text);
    }
}

void shell_handle_key(char c) {
    unsigned char key = (unsigned char)c;

    if (c == '\n') {
        on_enter();
    } else if (c == '\b') {
        if (line_backspace(&line))
            vga_put_char('\b');
    } else if (c == '\t') {
        on_tab();
    } else if (key == KEY_UP) {
        const char *older = history_older();
        if (older)
            show_line(older);
    } else if (key == KEY_DOWN && history_browsing()) {
        const char *newer = history_newer();
        show_line(newer ? newer : "");          // past the newest: empty line
    } else if (key >= ' ' && key < 0x7F) {      // printable: type it
        if (line_insert(&line, c))
            vga_put_char(c);
    }
    // Everything else (Escape, arrows left/right, ...) is ignored.
}

void shell_init(void) {
    command_count = 0;
    current_group = "Other";
    line_clear(&line);
}
