#include "gui_commands.h"
#include "shell.h"
#include "gui.h"
#include "keyboard.h"
#include "vga.h"
#include "bga.h"
#include "window_manager.h"

static void cmd_startx(const char *args) {
    (void)args;
    vga_print_str("Starting X11-like GUI... Press ESC to exit (if supported) or Reboot.\n");
    
    gui_init();
    wm_init();
    
    // Clear keyboard buffer
    keyboard_clear_last_key();
    
    // Create some test windows
    int win1 = wm_create_window(100, 150, 400, 250, "System Monitor");
    int win2 = wm_create_window(300, 200, 300, 200, "Hello UzOS");
    
    // Draw content inside the windows
    if (win1 >= 0) {
        wm_draw_string(win1, 10, 10, "CPU Usage: 4%", GUI_COLOR_BLACK, GUI_COLOR_WHITE);
        wm_draw_string(win1, 10, 30, "RAM Usage: 11 MB / 128 MB", GUI_COLOR_BLACK, GUI_COLOR_WHITE);
        wm_draw_string(win1, 10, 60, "Processes: 1", GUI_COLOR_BLACK, GUI_COLOR_WHITE);
        
        // Draw a fake graph
        wm_draw_rect(win1, 10, 100, 380, 100, GUI_COLOR_GREY);
        wm_draw_rect(win1, 10, 190, 380, 10, GUI_COLOR_GREEN);
    }
    
    if (win2 >= 0) {
        wm_draw_string(win2, 50, 50, "Welcome to the GUI!", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
        wm_draw_string(win2, 20, 80, "This is a real compositing", GUI_COLOR_BLACK, GUI_COLOR_WHITE);
        wm_draw_string(win2, 20, 100, "Window Manager!", GUI_COLOR_BLACK, GUI_COLOR_WHITE);
    }
    
    while (1) {
        wm_update();
        
        // Wait briefly so we don't cook the CPU
        for (volatile int i = 0; i < 50000; i++) {}
        
        // Exit condition
        if (keyboard_get_last_key() == 27) {
            gui_draw_rect(0, 0, 1024, 768, GUI_COLOR_BLACK);
            gui_draw_string(10, 10, "GUI terminated. Please reboot the OS.", GUI_COLOR_WHITE, GUI_COLOR_BLACK);
            gui_swap_buffers();
            while(1) { asm volatile("hlt"); }
        }
    }
}

void register_gui_commands(void) {
    shell_register_command("startx", "Start the Graphical User Interface (GUI)", cmd_startx);
}
