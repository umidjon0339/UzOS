#include "proc_syscalls.h"
#include "isr.h"
#include "vga.h"

// The generic Syscall Handler invoked by int 0x80
static void syscall_handler(registers_t *regs) {
    // In Linux, EAX contains the syscall number.
    // For now, let's just print that we received a syscall!
    vga_print_str("\n[KERNEL] System Call Received in Ring 0! Syscall Number: ");
    vga_print_int(regs->eax);
    vga_print_str("\n");
    
    // We will implement a switch case here for real syscalls like sys_write, sys_fork, etc.
}

void init_syscalls(void) {
    register_interrupt_handler(128, syscall_handler);
}
