#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

/*
 * fs_commands.h - Shell commands of filesystem_management.
 * Each command lives in its own folder: commands/<name>/cmd_<name>.c
 * Every path argument may be relative ("notes", "../x") or absolute ("/a/b").
 */

// Add every command below to the shell. Called once from kernel_main.
void register_fs_commands(void);

void cmd_ls(const char *args);
void cmd_cd(const char *args);
void cmd_pwd(const char *args);
void cmd_mkdir(const char *args);
void cmd_touch(const char *args);
void cmd_rm(const char *args);
void cmd_cat(const char *args);
void cmd_write(const char *args);
void cmd_cp(const char *args);
void cmd_mv(const char *args);
void cmd_wc(const char *args);
void cmd_grep(const char *args);
void cmd_hexdump(const char *args);

#endif
