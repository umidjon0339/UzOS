#ifndef PROC_COMMANDS_H
#define PROC_COMMANDS_H

/*
 * proc_commands.h - Add process_management's shell commands to the shell.
 *
 * Each command lives in its own folder, commands/<name>/, with its own
 * header cmd_<name>.h; proc_commands.c lists them all.
 */

// Add every command of process_management to the shell. Called once from kernel_main.
void register_proc_commands(void);

#endif
