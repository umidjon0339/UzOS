#ifndef PROC_COMMANDS_INTERNAL_H
#define PROC_COMMANDS_INTERNAL_H
#include "ps/cmd_ps.h"
void cmd_kill(const char *args);
void cmd_spawn(const char *args);
void cmd_sleep(const char *args);
void cmd_top(const char *args);
void cmd_exec(const char *args);
void cmd_testuser(const char *args);

int simple_atoi(const char *str);

#endif
