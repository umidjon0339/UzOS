/*
 * io_commands.c - Register the io commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "io_commands.h"
#include "shell.h"

void register_io_commands(void) {
    shell_begin_group("Input/Output");
    shell_register_command("ioinfo", "Open descriptors of a process: ioinfo [pid]", cmd_ioinfo);
}
