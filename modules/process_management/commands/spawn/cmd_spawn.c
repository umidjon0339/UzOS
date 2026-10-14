#include "../proc_commands_internal.h"
#include "process.h"
#include "vga.h"
#include "proc_syscalls.h"

static void dummy_worker(void) {
    int print_count = 0;
    while (1) {
        // Sleep for 3 seconds properly, giving CPU to other processes
        sys_sleep(3000);
        
        if (print_count < 3) {
            vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
            vga_print_str("\n[Worker] Background task heartbeat!\n> ");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            print_count++;
        } else {
            // Once done, let's just exit
            sys_exit();
        }
    }
}

void cmd_spawn(const char *args) {
    (void)args;
    int pid = process_create("worker", dummy_worker, 0);
    if (pid > 0) {
        vga_print_str("Spawned background worker with PID: ");
        vga_print_int(pid);
        vga_print_str("\n");
    } else {
        vga_print_str("Failed to spawn process.\n");
    }
}
