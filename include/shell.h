#ifndef SHELL_H
#define SHELL_H

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_COMMANDS 64
#define SHELL_HISTORY_SIZE 16

typedef void (*cmd_func_t)(const char *args);

typedef struct {
    const char *name;
    const char *description;
    cmd_func_t handler;
} shell_command_t;

void shell_init(void);
void shell_register_command(const char *name, const char *description, cmd_func_t handler);
void shell_handle_key(char c);
void shell_clear_buffer(void);

// Expose for tab completion
int shell_get_command_count(void);
shell_command_t *shell_get_commands(void);

#endif
