#ifndef DEVICE_COMMANDS_H
#define DEVICE_COMMANDS_H

/*
 * device_commands.h - Shell commands of device_management.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 */

// Add every command below to the shell. Called once from kernel_main.
void register_device_commands(void);

void cmd_uptime(const char *args);
void cmd_devices(const char *args);
void cmd_startx(const char *args);
void cmd_beep(const char *args);
void cmd_play(const char *args);
void cmd_date(const char *args);
void cmd_theme(const char *args);
void cmd_font(const char *args);
void cmd_mode(const char *args);

#endif
