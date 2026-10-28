#ifndef CPU_COMMANDS_H
#define CPU_COMMANDS_H

/*
 * cpu_commands.h - Add cpu_management's shell commands to the shell.
 *
 * Each command lives in its own folder, commands/<name>/, with its own
 * header cmd_<name>.h; cpu_commands.c lists them all.
 */

// Add every command of cpu_management to the shell. Called once from kernel_main.
void register_cpu_commands(void);

#endif
