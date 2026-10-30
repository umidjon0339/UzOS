#ifndef CMD_EXEC_H
#define CMD_EXEC_H

/*
 * cmd_exec.h - `exec <file> [&]`
 *
 * Load an ELF program and run it in the foreground or (with &) the background.
 */

void cmd_exec(const char *args);

#endif
