#ifndef USER_COMMANDS_H
#define USER_COMMANDS_H

/*
 * user_commands.h - Shell commands of user_management.
 */

// Add every command below to the shell. Called once from kernel_main.
void register_user_commands(void);

void cmd_whoami(const char *args);
void cmd_id(const char *args);
void cmd_users(const char *args);
void cmd_su(const char *args);
void cmd_logout(const char *args);
void cmd_passwd(const char *args);
void cmd_chmod(const char *args);
void cmd_chown(const char *args);
void cmd_useradd(const char *args);
void cmd_userdel(const char *args);

#endif
