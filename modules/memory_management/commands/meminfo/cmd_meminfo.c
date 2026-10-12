#include "../mem_commands_internal.h"
#include "pmm.h"
#include "vga.h"

void cmd_meminfo(const char *args) {
    (void)args;
    uint32_t max_blocks  = pmm_get_max_blocks();
    uint32_t used_blocks = pmm_get_used_blocks();
    uint32_t free_blocks = pmm_get_free_blocks();

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n--- Physical Memory Status ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_print_str("Total Physical RAM : ");
    vga_print_int((max_blocks * 4) / 1024);
    vga_print_str(" MB (");
    vga_print_int(max_blocks);
    vga_print_str(" Page Frames)\n");

    vga_print_str("Used Page Frames   : ");
    vga_print_int(used_blocks);
    vga_print_str(" (");
    vga_print_int((used_blocks * 4) / 1024);
    vga_print_str(" MB)\n");

    vga_print_str("Free Page Frames   : ");
    vga_print_int(free_blocks);
    vga_print_str(" (");
    vga_print_int((free_blocks * 4) / 1024);
    vga_print_str(" MB)\n");

    uint32_t usage_pct = (used_blocks * 100) / max_blocks;
    vga_print_str("Memory Usage       : ");
    vga_print_int(usage_pct);
    vga_print_str("%\n");
}
