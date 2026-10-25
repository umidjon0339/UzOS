#ifndef SECURITY_COMMANDS_H
#define SECURITY_COMMANDS_H

/*
 * security_commands.h - Shell commands of security_management.
 */

// Add every command below to the shell. Called once from kernel_main.
void register_security_commands(void);

void cmd_seclab(const char *args);
void cmd_audit(const char *args);
void cmd_caps(const char *args);
void cmd_integrity(const char *args);

#endif
