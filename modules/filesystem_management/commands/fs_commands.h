#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

/*
 * fs_commands.h - Add filesystem_management's shell commands to the shell.
 *
 * Each command lives in its own folder, commands/<name>/, with its own
 * header cmd_<name>.h; fs_commands.c lists them all. Every path argument
 * may be relative ("notes", "../x") or absolute ("/a/b").
 */

// Add every command of filesystem_management to the shell. Called once from kernel_main.
void register_fs_commands(void);

#endif
