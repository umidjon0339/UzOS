#include "terminal.h"

void kernel_main(void) {
    // Initialize the VGA terminal driver (clears screen, sets color)
    terminal_initialize();

    // Print the welcome message exactly as requested
    terminal_print("================================\n");
    terminal_print("          Welcome to MyOS\n");
    terminal_print("================================\n\n");
    
    terminal_print("MyOS v0.1\n");
    terminal_print("Kernel successfully started!\n\n");
    
    terminal_print("Architecture: x86_64\n");
    terminal_print("Status: OK\n\n");
    
    terminal_print("System halted safely.\n");

    // Once kernel_main returns, the assembly code in boot64.S will execute
    // an infinite safe halt loop (`hlt` instruction).
}
