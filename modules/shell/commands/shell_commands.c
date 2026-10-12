#include "shell_commands.h"
#include "shell.h"
#include "vga.h"

// Shell-owned command: help
// Lists all registered commands and their descriptions
static void cmd_help(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n--- General Commands ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("  help       - Show this help message\n");
    vga_print_str("  clear      - Clear the screen\n");
    vga_print_str("  echo <msg> - Print a message\n");
    vga_print_str("  uptime     - Show system uptime\n");
    vga_print_str("  color <c>  - Change text color\n");
    vga_print_str("  calc <exp> - Simple calculator\n");
    
    vga_print_str("\n--- Memory Management ---\n");
    vga_print_str("  meminfo    - Show physical memory status\n");
    vga_print_str("  heapinfo   - Show kernel heap status\n");

    vga_print_str("\n--- Process Management ---\n");
    vga_print_str("  ps         - List running processes\n");
    vga_print_str("  top        - Live view of processes (press 'q' to quit)\n");
    vga_print_str("  spawn      - Spawn a background worker thread\n");
    vga_print_str("  kill <pid> - Kill a process by PID\n");
    vga_print_str("  sleep <N>  - Sleep for N seconds\n");

    vga_print_str("\n--- Virtual Filesystem (RAMDisk) ---\n");
    vga_print_str("  pwd        - Print working directory\n");
    vga_print_str("  ls         - List files and directories\n");
    vga_print_str("  cd <dir>   - Change working directory\n");
    vga_print_str("  mkdir <n>  - Create a directory\n");
    vga_print_str("  touch <f>  - Create an empty file\n");
    vga_print_str("  write <f> <t> - Write text to a file\n");
    vga_print_str("  cat <f>    - Print file contents\n");
    vga_print_str("  rm <f>     - Delete a file or directory\n");
    vga_print_str("  cp <s> <d> - Copy a file\n");
    vga_print_str("  mv <s> <d> - Move/rename a file\n");
    vga_print_str("  wc <f>     - Word, line, and character count\n");
    vga_print_str("  grep <p> <f>- Search for pattern in file\n");
    vga_print_str("  hexdump <f>- Hexadecimal view of file\n");

    vga_print_str("\n--- Audio & Entertainment ---\n");
    vga_print_str("  beep [hz]  - Play a tone on PC speaker\n");
    vga_print_str("  play       - Play UzOS jingle!\n");
    vga_print_str("  snake      - Play the Snake game!\n");

    vga_print_str("\n--- Graphical User Interface ---\n");
    vga_print_str("  startx     - Start the GUI Desktop Environment\n");
    
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_print_str("\n  TIP: Use Tab for auto-complete, Up/Down for history\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

// Shell-owned command: clear
// Clears the VGA display
static void cmd_clear(const char *args) {
    (void)args;
    vga_init();
}

// Shell-owned command: echo
// Prints user-provided text back to the screen
static void cmd_echo(const char *args) {
    vga_print_str("\n");
    if (args[0] != '\0') {
        vga_print_str(args);
    }
    vga_print_str("\n");
}

// External command handlers (defined in their own .c files)
extern void cmd_color(const char *args);
extern void cmd_calc(const char *args);
extern void cmd_snake(const char *args);
extern void cmd_beep(const char *args);
extern void cmd_play(const char *args);

void register_shell_commands(void) {
    shell_register_command("help",  "Show available commands", cmd_help);
    shell_register_command("clear", "Clear the screen",        cmd_clear);
    shell_register_command("echo",  "Print a message",         cmd_echo);
    shell_register_command("color", "Change text color",       cmd_color);
    shell_register_command("calc",  "Simple calculator",       cmd_calc);
    shell_register_command("snake", "Play the Snake game!",    cmd_snake);
    shell_register_command("beep",  "Play PC speaker tone",    cmd_beep);
    shell_register_command("play",  "Play jingle melody",      cmd_play);
}
