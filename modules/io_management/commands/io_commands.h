#ifndef IO_COMMANDS_H
#define IO_COMMANDS_H

/*
 * io_commands.h - Shell commands of io_management.
 */

// Add every command below to the shell. Called once from kernel_main.
void register_io_commands(void);

void cmd_ioinfo(const char *args);

#endif
