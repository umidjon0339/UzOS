#ifndef SHELL_H
#define SHELL_H

/*
 * shell.h - The command line: read a line, find the command, run it.
 *
 * The shell knows no commands of its own. Every module registers its
 * commands at boot (each module's register_*_commands()), under a group
 * name that `help` uses to sort them.
 */

#define SHELL_MAX_COMMANDS 256

typedef void (*cmd_func_t)(const char *args);

typedef struct {
    const char *name;           // what you type: "ls"
    const char *description;    // one line for `help ls`
    const char *group;          // "Files", "Memory", ...
    cmd_func_t handler;         // called with the rest of the line
} shell_command_t;

void shell_init(void);

// Commands registered after this belong to group 'title'.
void shell_begin_group(const char *title);
void shell_register_command(const char *name, const char *description, cmd_func_t handler);

// Run one command line ("ls -l") in the CALLING process: find the
// command and call it. 1 = ran, 0 = no such command (an error is printed).
// The prompt uses it; so does cpu_management/core/launch, which runs a
// line inside a child process (nice, chrt, taskset, time, ...).
int shell_run_command(const char *input);

// Feed one typed key to the shell (called from the kernel_main loop).
void shell_handle_key(char c);

void shell_print_prompt(void);

// The registry, for `help` and Tab completion.
int shell_command_count(void);
const shell_command_t *shell_command(int index);
const shell_command_t *shell_find(const char *name);

#endif
