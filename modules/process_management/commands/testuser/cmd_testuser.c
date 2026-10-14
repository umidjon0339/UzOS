#include "../proc_commands_internal.h"
#include "vga.h"
#include "proc_security.h"
#include <stdint.h>

extern void switch_to_user_mode(uint32_t eip, uint32_t esp);

static void user_mode_function(void) {
    // We are now in Ring 3!
    // We cannot call vga_print_str directly because it accesses hardware memory which might cause a Page Fault if we protected it.
    // Let's test the Syscall instead!
    asm volatile("mov $42, %%eax; int $0x80" : : : "eax"); // Send syscall 42
    
    // We can't exit cleanly yet without a sys_exit syscall.
    while(1) {}
}

void cmd_testuser(const char *args) {
    (void)args;
    vga_print_str("Switching to Ring 3 User Mode...\n");
    
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));

    /* DEFENCE user_stack: ring 3 must never be handed a kernel stack.
     * The original code passed the kernel's own ESP here, which lets
     * user code scribble over kernel memory. */
    if (!def_user_stack_ok(esp)) {
        vga_print_str("[SECURITY] Refused: a kernel stack cannot be given to ring 3.\n");
        return;
    }

    // WARNING: This will never return! The thread will jump to Ring 3 and spin forever after testing the syscall.
    // A true implementation spawns a new process for this.
    switch_to_user_mode((uint32_t)user_mode_function, esp);
}
