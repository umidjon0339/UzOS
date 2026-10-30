#ifndef NET_COMMANDS_H
#define NET_COMMANDS_H

/*
 * net_commands.h - Add network_management's shell commands to the shell.
 *
 * Each command lives in its own folder, commands/<name>/, with its own
 * header cmd_<name>.h; net_commands.c lists them all.
 */

// Add every command of network_management to the shell. Called once from kernel_main.
void register_net_commands(void);

#endif
