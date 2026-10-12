#ifndef FS_COMMANDS_INTERNAL_H
#define FS_COMMANDS_INTERNAL_H

void cmd_ls(const char *args);
void cmd_mkdir(const char *args);
void cmd_touch(const char *args);
void cmd_write(const char *args);
void cmd_cat(const char *args);
void cmd_rm(const char *args);
void cmd_cd(const char *args);
void cmd_pwd(const char *args);
void cmd_cp(const char *args);
void cmd_mv(const char *args);
void cmd_wc(const char *args);
void cmd_grep(const char *args);
void cmd_hexdump(const char *args);

#endif

