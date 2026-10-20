/*
 * cmd_whoami.c - `whoami`: the name of the user this shell runs as.
 *
 * The shell is a process like any other; its uid is "who you are".
 *
 * Uses: process_caller_uid (process_management), users_name (core/users).
 */

#include "user_commands.h"
#include "process.h"
#include "users.h"
#include "vga.h"

void cmd_whoami(const char *args) {
    (void)args;
    char name[USER_NAME_MAX + 2];
    users_name(process_caller_uid(), name, sizeof(name));
    vga_print_str(name);
    vga_print_str("\n");
}
