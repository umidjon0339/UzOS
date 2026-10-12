#include "proc_security.h"
#include "vga.h"

void proc_security_check_stack_overflow(process_t *p) {
    // If the stack pointer goes below the dynamically allocated stack base, it's an overflow.
    if (p && p->esp < p->stack_base) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("[SECURITY] Stack overflow detected in PID ");
        vga_print_int(p->pid);
        vga_print_str("!\n");
        while(1) { asm volatile("hlt"); }
    }
}
