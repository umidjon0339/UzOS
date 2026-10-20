#ifndef CPU_COMMANDS_H
#define CPU_COMMANDS_H

/*
 * cpu_commands.h - Shell commands of cpu_management.
 */

// Add every command below to the shell. Called once from kernel_main.
void register_cpu_commands(void);

void cmd_cpuinfo(const char *args);
void cmd_faults(const char *args);
void cmd_cpustat(const char *args);
void cmd_nice(const char *args);

#endif
