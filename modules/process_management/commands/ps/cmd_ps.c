#include "cmd_ps.h"        // modules/process_management/commands/ps/cmd_ps.h
#include "process.h"     // include/process.h
#include "shell.h"     // include/shell.h
#include "vga.h"         // include/vga.h

// Convert process state enum to string
static const char *ps_state_to_str(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:    return "READY   ";
        case PROCESS_STATE_RUNNING:  return "RUNNING ";
        case PROCESS_STATE_SLEEPING: return "SLEEPING";
        case PROCESS_STATE_ZOMBIE:   return "ZOMBIE  ";
        default:                     return "UNKNOWN ";
    }
}

// Command handler: list active processes
void cmd_ps(const char *args) {
    (void)args;
    process_t *ptable = process_get_table();
    if (!ptable) return;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("PID\tSTATE\t\tNAME\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (ptable[i].state != PROCESS_STATE_FREE) {
            vga_print_int(ptable[i].pid);
            vga_print_str("\t");
            vga_print_str(ps_state_to_str(ptable[i].state));
            vga_print_str("\t");
            vga_print_str(ptable[i].name);
            vga_print_str("\n");
        }
    }
}

// Register ps command into shell
void cmd_ps_register(void) {
    shell_register_command(CMD_PS_NAME, CMD_PS_DESC, cmd_ps);
}
