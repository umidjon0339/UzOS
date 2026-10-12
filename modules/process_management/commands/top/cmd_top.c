#include "../proc_commands_internal.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "proc_syscalls.h"

void cmd_top(const char *args) {
    (void)args;
    keyboard_clear_last_key();
    
    while (1) {
        if (keyboard_get_last_key() == 'q') {
            break;
        }
        
        vga_init(); // Clear screen
        vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
        vga_print_str("UzOS Top - Press 'q' to quit\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_print_str("=====================================\n");
        
        // Call our ps logic
        cmd_ps("");
        
        // Sleep for 1 second before refreshing
        sys_sleep(1000);
    }
    
    // Cleanup so 'q' doesn't execute in shell
    keyboard_clear_last_key();
    shell_clear_buffer();
    vga_init(); // Clear screen again for a fresh shell prompt
}
