#ifndef MEM_COMMANDS_H
#define MEM_COMMANDS_H

/*
 * mem_commands.h - Shell commands of memory_management.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 */

// Add every command below to the shell. Called once from kernel_main.
void register_mem_commands(void);

void cmd_meminfo(const char *args);
void cmd_heapinfo(const char *args);
void cmd_pagewalk(const char *args);
void cmd_memtest(const char *args);

#endif
