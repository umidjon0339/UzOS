#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "string.h"
#include "io.h"

#define MAX_CMD_LEN 256
#define NUM_COMMANDS 6

/* Forward declarations for command handlers */
static void cmd_help(const char* args);
static void cmd_man(const char* args);
static void cmd_clear(const char* args);
static void cmd_echo(const char* args);
static void cmd_version(const char* args);
static void cmd_reboot(const char* args);

/* Command entry: name, short description, manual text, handler */
struct command {
    const char* name;
    const char* desc;
    const char* manual;
    void (*handler)(const char* args);
};

static const struct command commands[NUM_COMMANDS] = {
    {
        "help",
        "List all available commands",
        "HELP\n\n"
        "  Usage: help\n\n"
        "  Displays a list of all built-in commands\n"
        "  with a short description for each one.\n",
        cmd_help
    },
    {
        "man",
        "Show manual for a command",
        "MAN\n\n"
        "  Usage: man <command>\n\n"
        "  Displays the detailed manual page for\n"
        "  the specified command.\n\n"
        "  Example: man echo\n",
        cmd_man
    },
    {
        "clear",
        "Clear the screen",
        "CLEAR\n\n"
        "  Usage: clear\n\n"
        "  Clears the terminal screen and moves\n"
        "  the cursor to the top-left corner.\n",
        cmd_clear
    },
    {
        "echo",
        "Print text to the screen",
        "ECHO\n\n"
        "  Usage: echo <text>\n\n"
        "  Prints the given text to the terminal.\n\n"
        "  Example: echo hello world\n",
        cmd_echo
    },
    {
        "version",
        "Show OS version info",
        "VERSION\n\n"
        "  Usage: version\n\n"
        "  Displays the current version of MyOS\n"
        "  and basic system information.\n",
        cmd_version
    },
    {
        "reboot",
        "Reboot the system",
        "REBOOT\n\n"
        "  Usage: reboot\n\n"
        "  Performs a system reboot using the\n"
        "  keyboard controller reset method.\n"
        "  Warning: all unsaved state is lost.\n",
        cmd_reboot
    }
};

/* --- Command handlers --- */

static void cmd_help(const char* args) {
    (void)args;
    terminal_print("Available commands:\n\n");
    for (int i = 0; i < NUM_COMMANDS; i++) {
        terminal_print("  ");
        terminal_print(commands[i].name);
        /* Pad with spaces for alignment */
        size_t len = strlen(commands[i].name);
        for (size_t j = len; j < 10; j++) terminal_putchar(' ');
        terminal_print(commands[i].desc);
        terminal_putchar('\n');
    }
    terminal_putchar('\n');
}

static void cmd_man(const char* args) {
    /* Skip leading spaces */
    while (*args == ' ') args++;

    if (*args == '\0') {
        terminal_print("Usage: man <command>\n");
        return;
    }

    /* Find the command */
    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(args, commands[i].name) == 0) {
            terminal_putchar('\n');
            terminal_print(commands[i].manual);
            return;
        }
    }
    terminal_print("man: no manual for '");
    terminal_print(args);
    terminal_print("'\n");
}

static void cmd_clear(const char* args) {
    (void)args;
    terminal_clear();
}

static void cmd_echo(const char* args) {
    /* Skip one leading space after command name */
    if (*args == ' ') args++;
    terminal_print(args);
    terminal_putchar('\n');
}

static void cmd_version(const char* args) {
    (void)args;
    terminal_print("MyOS v0.1\n");
    terminal_print("Architecture: x86_64\n");
    terminal_print("Built with: GCC + NASM\n");
}

static void cmd_reboot(const char* args) {
    (void)args;
    terminal_print("Rebooting...\n");
    /* Triple-fault reboot: pulse the keyboard controller reset line */
    outb(0x64, 0xFE);
    /* If that didn't work, halt */
    __asm__ volatile ("cli; hlt");
}

/* --- Shell main loop --- */

/* Find command by name, return index or -1 */
static int find_command(const char* name) {
    for (int i = 0; i < NUM_COMMANDS; i++) {
        size_t len = strlen(commands[i].name);
        /* Match command name (followed by space or end-of-string) */
        if (strncmp(name, commands[i].name, len) == 0 &&
            (name[len] == ' ' || name[len] == '\0')) {
            return i;
        }
    }
    return -1;
}

/* Print the shell prompt */
static void print_prompt(void) {
    terminal_print("myos> ");
}

void shell_run(void) {
    char cmd_buf[MAX_CMD_LEN];
    int cmd_len = 0;

    print_prompt();

    while (1) {
        char c = keyboard_get_char();
        if (c == 0) {
            /* No key pressed — idle wait */
            __asm__ volatile ("hlt");
            continue;
        }

        if (c == '\n') {
            /* Enter pressed — process the command */
            terminal_putchar('\n');
            cmd_buf[cmd_len] = '\0';

            if (cmd_len > 0) {
                int idx = find_command(cmd_buf);
                if (idx >= 0) {
                    /* Pass everything after command name as args */
                    const char* args = cmd_buf + strlen(commands[idx].name);
                    commands[idx].handler(args);
                } else {
                    terminal_print("Unknown command: ");
                    terminal_print(cmd_buf);
                    terminal_print("\nType 'help' to see available commands.\n");
                }
            }

            cmd_len = 0;
            print_prompt();

        } else if (c == '\b') {
            /* Backspace — remove last character */
            if (cmd_len > 0) {
                cmd_len--;
                terminal_putchar('\b');
            }

        } else {
            /* Normal character — echo and buffer it */
            if (cmd_len < MAX_CMD_LEN - 1) {
                cmd_buf[cmd_len++] = c;
                terminal_putchar(c);
            }
        }
    }
}
