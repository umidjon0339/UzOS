#include "timer_commands.h"
#include "shell.h"
#include "timer.h"
#include "vga.h"

// Timer-owned command: uptime
// Shows system uptime based on PIT ticks.
static void cmd_uptime(const char *args) {
    (void)args;
    uint32_t ticks = timer_ticks;
    uint32_t seconds = ticks / 100; // PIT runs at 100 Hz
    uint32_t minutes = seconds / 60;
    uint32_t hours   = minutes / 60;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n--- System Uptime ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_print_str("Uptime   : ");
    vga_print_int(hours);
    vga_print_str("h ");
    vga_print_int(minutes % 60);
    vga_print_str("m ");
    vga_print_int(seconds % 60);
    vga_print_str("s\n");

    vga_print_str("PIT Ticks: ");
    vga_print_int(ticks);
    vga_print_str("\n");
}

// Register timer commands with the shell dispatcher
void register_timer_commands(void) {
    shell_register_command("uptime", "Show system uptime", cmd_uptime);
}
