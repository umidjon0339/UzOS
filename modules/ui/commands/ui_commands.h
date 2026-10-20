#ifndef UI_COMMANDS_H
#define UI_COMMANDS_H

/*
 * ui_commands.h - Shell commands of the ui module.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 */

// Add every command below to the shell. Called once from kernel_main.
void register_ui_commands(void);

void cmd_uzfetch(const char *args);
void cmd_statusbar(const char *args);

#endif
