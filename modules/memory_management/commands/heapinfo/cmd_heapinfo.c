#include "../mem_commands_internal.h"
#include "kheap.h"
#include "vga.h"

void cmd_heapinfo(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n--- Kernel Heap Status ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_print_str("Heap Start Virtual Addr : ");
    vga_print_hex(KHEAP_START);
    vga_print_str("\nHeap Total Capacity    : ");
    vga_print_int(KHEAP_INITIAL_SIZE / 1024);
    vga_print_str(" KB\n");
}
