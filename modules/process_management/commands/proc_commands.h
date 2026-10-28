#ifndef PROC_COMMANDS_H
#define PROC_COMMANDS_H

/*
 * proc_commands.h - Shell commands of process_management.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 */

// Add every command below to the shell. Called once from kernel_main.
void register_proc_commands(void);

// ps and top moved to cpu_management/commands/ (with the other CPU tools).
void cmd_kill(const char *args);
void cmd_spawn(const char *args);
void cmd_sleep(const char *args);
void cmd_exec(const char *args);
void cmd_testuser(const char *args);

#endif
