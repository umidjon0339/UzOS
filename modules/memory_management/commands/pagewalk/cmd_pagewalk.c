/*
 * cmd_pagewalk.c - `pagewalk <addr>`: follow an address through paging.
 *
 * Shows exactly what the CPU does for every memory access: split the
 * address into directory index, table index and offset, read both
 * entries, and build the physical address. Try:
 *
 *   pagewalk 0xB8000      VGA text memory (identity mapped)
 *   pagewalk 0x1000000    the kernel heap (mapped elsewhere)
 *   pagewalk 0            the NULL page (deliberately not mapped)
 *
 * Uses: vmm_walk (core/vmm), strtoul (lib/stdlib), ui_* (ui module).
 */

#include "mem_commands.h"
#include "vmm.h"
#include "stdlib.h"
#include "ui.h"

static void print_entry(const char *label, uint32_t entry) {
    ui_label(label, 12);
    vga_print_hex(entry);
    if (!(entry & PAGE_PRESENT)) {
        ui_color(UI_BAD);
        vga_print_str("  not present");
        ui_end();
        return;
    }
    ui_color(UI_GOOD);
    vga_print_str("  present");
    ui_color(UI_DIM);
    vga_print_str(entry & PAGE_WRITABLE ? " writable" : " read-only");
    vga_print_str(entry & PAGE_USER ? " user" : " kernel-only");
    ui_end();
}

void cmd_pagewalk(const char *args) {
    if (args[0] == '\0') {
        ui_usage("pagewalk <address>   e.g. pagewalk 0xB8000");
        return;
    }

    uint32_t virt = strtoul(args, 0, 0);
    vmm_walk_t w;
    int mapped = vmm_walk(virt, &w);

    ui_label("virtual", 12);
    vga_print_hex(virt);
    vga_print_str(" = directory #");
    vga_print_int(w.dir_index);
    vga_print_str(", table #");
    vga_print_int(w.table_index);
    vga_print_str(", offset ");
    vga_print_int(virt & PAGE_FLAGS);
    vga_print_str("\n");

    print_entry("dir entry", w.pde);
    if (w.pde & PAGE_PRESENT)
        print_entry("table entry", w.pte);

    if (mapped) {
        ui_label("physical", 12);
        ui_color(UI_GOOD);
        vga_print_hex(w.physical);
        ui_end();
    } else {
        ui_warn(0, "not mapped: an access here is a page fault");
    }
}
