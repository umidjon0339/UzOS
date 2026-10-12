#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "vfs.h"
#include "fs_syscalls.h"

// Line input buffer
static char shell_buffer[SHELL_BUFFER_SIZE];
static int  shell_buffer_pos = 0;

// Command registry
static shell_command_t commands[SHELL_MAX_COMMANDS];
static int command_count = 0;

// Command history (circular buffer)
static char history[SHELL_HISTORY_SIZE][SHELL_BUFFER_SIZE];
static int history_count = 0;    // Total commands stored
static int history_write = 0;    // Next write position (circular)
static int history_browse = -1;  // Current browse position (-1 = not browsing)

// Simple strcmp implementation (no libc in freestanding)
static int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == *b);
}

// Check if string starts with a prefix
static int str_starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

// Dynamically print the prompt based on CWD
static void print_prompt(void) {
    char cwd[128];
    
    if (sys_getcwd_string(cwd, 128) == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print_str(cwd);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print_str("> ");
    } else {
        vga_print_str("> ");
    }
}

// Erase the current line content from the screen
static void erase_current_input(void) {
    // Backspace over each character currently displayed
    for (int i = 0; i < shell_buffer_pos; i++) {
        vga_put_char('\b');
    }
}

// Display the current buffer on screen
static void display_current_input(void) {
    for (int i = 0; i < shell_buffer_pos; i++) {
        vga_put_char(shell_buffer[i]);
    }
}

// Replace current input with a string (used for history navigation)
static void replace_input(const char *str) {
    erase_current_input();
    shell_buffer_pos = 0;
    while (*str && shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
        shell_buffer[shell_buffer_pos++] = *str++;
    }
    shell_buffer[shell_buffer_pos] = '\0';
    display_current_input();
}

// Add a command to history
static void history_add(const char *cmd) {
    if (cmd[0] == '\0') return; // Don't store empty commands
    
    // Don't store duplicate of the last command
    if (history_count > 0) {
        int last = (history_write - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
        if (str_equal(history[last], cmd)) return;
    }
    
    strcpy(history[history_write], cmd);
    history_write = (history_write + 1) % SHELL_HISTORY_SIZE;
    if (history_count < SHELL_HISTORY_SIZE) {
        history_count++;
    }
}

void shell_register_command(const char *name, const char *description, cmd_func_t handler) {
    if (command_count >= SHELL_MAX_COMMANDS) return;

    commands[command_count].name        = name;
    commands[command_count].description = description;
    commands[command_count].handler     = handler;
    command_count++;
}

int shell_get_command_count(void) {
    return command_count;
}

shell_command_t *shell_get_commands(void) {
    return commands;
}

static void shell_execute(const char *input) {
    // Skip empty input
    if (input[0] == '\0') {
        print_prompt();
        return;
    }

    // Extract command name (first word before space)
    char cmd_name[64];
    const char *args = "";
    int i = 0;
    while (input[i] && input[i] != ' ' && i < 63) {
        cmd_name[i] = input[i];
        i++;
    }
    cmd_name[i] = '\0';

    // If there are arguments after the space, point to them
    if (input[i] == ' ') {
        args = &input[i + 1];
    }

    // Search registered commands
    for (int c = 0; c < command_count; c++) {
        if (str_equal(cmd_name, commands[c].name)) {
            commands[c].handler(args);
            print_prompt();
            return;
        }
    }

    // Command not found
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print_str("Unknown command: ");
    vga_print_str(cmd_name);
    vga_print_str("\nType 'help' for available commands.\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_prompt();
}

// Tab completion: find matching commands and filenames
static void handle_tab(void) {
    shell_buffer[shell_buffer_pos] = '\0';
    
    // Only complete the first word (command name) if no space yet
    // If there's a space, try to complete filenames
    int has_space = 0;
    for (int i = 0; i < shell_buffer_pos; i++) {
        if (shell_buffer[i] == ' ') { has_space = 1; break; }
    }
    
    if (!has_space) {
        // Command completion
        const char *prefix = shell_buffer;
        int prefix_len = shell_buffer_pos;
        
        if (prefix_len == 0) return; // Nothing to complete
        
        // Find all matching commands
        const char *match = 0;
        int match_count = 0;
        
        for (int i = 0; i < command_count; i++) {
            if (str_starts_with(commands[i].name, prefix)) {
                match = commands[i].name;
                match_count++;
            }
        }
        
        if (match_count == 1) {
            // Unique match — complete it
            replace_input(match);
            // Add a space after the completed command
            if (shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
                shell_buffer[shell_buffer_pos++] = ' ';
                vga_put_char(' ');
            }
        } else if (match_count > 1) {
            // Multiple matches — show them
            vga_print_str("\n");
            vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
            for (int i = 0; i < command_count; i++) {
                if (str_starts_with(commands[i].name, prefix)) {
                    vga_print_str(commands[i].name);
                    vga_print_str("  ");
                }
            }
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_print_str("\n");
            print_prompt();
            display_current_input();
        }
    } else {
        // Filename completion — complete the last word
        // Find the start of the last word (after the last space)
        int last_space = 0;
        for (int i = 0; i < shell_buffer_pos; i++) {
            if (shell_buffer[i] == ' ') last_space = i + 1;
        }
        
        const char *file_prefix = &shell_buffer[last_space];
        int file_prefix_len = shell_buffer_pos - last_space;
        
        if (file_prefix_len == 0) return;
        
        // List files in CWD and find matches
        vfs_node_t *cwd = sys_getcwd_node();
        if (!cwd) return;
        
        const char *file_match = 0;
        int file_match_count = 0;
        
        for (int i = 0; i < cwd->num_children; i++) {
            if (str_starts_with(cwd->children[i]->name, file_prefix)) {
                file_match = cwd->children[i]->name;
                file_match_count++;
            }
        }
        
        if (file_match_count == 1) {
            // Complete the filename
            // Erase current input, rebuild with completed filename
            erase_current_input();
            shell_buffer_pos = last_space;
            // Copy the matched filename
            int j = 0;
            while (file_match[j] && shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
                shell_buffer[shell_buffer_pos++] = file_match[j++];
            }
            shell_buffer[shell_buffer_pos] = '\0';
            display_current_input();
        } else if (file_match_count > 1) {
            // Show all matches
            vga_print_str("\n");
            vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
            for (int i = 0; i < cwd->num_children; i++) {
                if (str_starts_with(cwd->children[i]->name, file_prefix)) {
                    vga_print_str(cwd->children[i]->name);
                    if (cwd->children[i]->flags == FS_DIRECTORY) {
                        vga_print_str("/");
                    }
                    vga_print_str("  ");
                }
            }
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_print_str("\n");
            print_prompt();
            display_current_input();
        }
    }
}

void shell_handle_key(char c) {
    if (c == '\n') {
        // Enter pressed: terminate buffer and execute
        vga_put_char('\n');
        shell_buffer[shell_buffer_pos] = '\0';
        history_add(shell_buffer);
        history_browse = -1; // Reset browse position
        shell_execute(shell_buffer);
        shell_buffer_pos = 0;
    } else if (c == '\b') {
        // Backspace: remove last character
        if (shell_buffer_pos > 0) {
            shell_buffer_pos--;
            vga_put_char('\b');
        }
    } else if (c == '\t') {
        // Tab: auto-complete
        handle_tab();
    } else if ((unsigned char)c == KEY_UP) {
        // Up arrow: browse history backward
        if (history_count == 0) return;
        
        if (history_browse == -1) {
            // Start browsing from the most recent command
            history_browse = (history_write - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
        } else {
            // Go further back
            int prev = (history_browse - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
            // Don't wrap past the oldest entry
            int oldest = (history_count < SHELL_HISTORY_SIZE) ? 0 : history_write;
            if (prev != ((oldest - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE)) {
                history_browse = prev;
            } else {
                return; // Already at oldest
            }
        }
        replace_input(history[history_browse]);
    } else if ((unsigned char)c == KEY_DOWN) {
        // Down arrow: browse history forward
        if (history_browse == -1) return;
        
        int newest = (history_write - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
        if (history_browse == newest) {
            // We're at the newest — clear input
            history_browse = -1;
            erase_current_input();
            shell_buffer_pos = 0;
        } else {
            history_browse = (history_browse + 1) % SHELL_HISTORY_SIZE;
            replace_input(history[history_browse]);
        }
    } else if ((unsigned char)c >= 0x80) {
        // Other special keys (left, right, etc.) — ignore for now
        return;
    } else {
        // Normal character: add to buffer and echo
        if (shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
            shell_buffer[shell_buffer_pos++] = c;
            vga_put_char(c);
        }
    }
}

void shell_init(void) {
    shell_buffer_pos = 0;
    command_count = 0;
    history_count = 0;
    history_write = 0;
    history_browse = -1;
}

void shell_clear_buffer(void) {
    shell_buffer_pos = 0;
}
