#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

void register_shell_commands(void);

// Individual command handlers (declared here for cross-file access)
void cmd_color(const char *args);
void cmd_calc(const char *args);
void cmd_snake(const char *args);
void cmd_beep(const char *args);
void cmd_play(const char *args);

#endif
