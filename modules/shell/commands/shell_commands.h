#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

/*
 * shell_commands.h - The shell's own commands.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 */

// Add every command below to the shell. Called once from kernel_main.
void register_shell_commands(void);

void cmd_help(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_color(const char *args);
void cmd_calc(const char *args);
void cmd_history(const char *args);
void cmd_snake(const char *args);

#endif
