/*
 * cmd_meminfo.c - `meminfo`: how much physical RAM is used and free.
 *
 * Also prints the memory map the bootloader gave us: which address
 * ranges are real usable RAM and which belong to firmware.
 *
 * Uses: pmm_get_*_blocks and pmm_region (core/pmm), box_* / ui_bar
 *       (ui module).
 */

#include "mem_commands.h"
#include "pmm.h"
#include "ui.h"
#include "box.h"

#define KB_PER_FRAME (PAGE_SIZE / 1024)
#define WIDTH        58

static const char *region_type(uint32_t type) {
    switch (type) {
        case MULTIBOOT_MEMORY_AVAILABLE:        return "usable RAM";
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: return "ACPI tables";
        case MULTIBOOT_MEMORY_NVS:              return "ACPI NVS";
        case MULTIBOOT_MEMORY_BADRAM:           return "bad RAM";
        default:                                return "reserved";
    }
}

static void frames_row(const char *label, uint32_t frames) {
    box_row();
    ui_label(label, 8);
    ui_printf("%6u frames  %4u MB", frames, frames * KB_PER_FRAME / 1024);
    box_row_end();
}

void cmd_meminfo(const char *args) {
    (void)args;
    uint32_t total = pmm_get_max_blocks();
    uint32_t used  = pmm_get_used_blocks();

    box_begin("Physical memory (4 KB frames)", WIDTH);
    box_row();
    ui_label("Usage", 8);
    ui_bar(used, total, 30);
    ui_printf(" %3u%%", total ? used * 100 / total : 0);
    box_row_end();
    frames_row("Total", total);
    frames_row("Used", used);
    frames_row("Free", pmm_get_free_blocks());

    box_divider();
    box_row();
    ui_color(UI_TITLE);
    vga_print_str("Memory map from the bootloader");
    box_row_end();
    for (int i = 0; i < pmm_region_count(); i++) {
        const pmm_region_t *r = pmm_region(i);
        int usable = r->type == MULTIBOOT_MEMORY_AVAILABLE;
        box_row();
        vga_print_hex((uint32_t)r->base);
        vga_print_str(" - ");
        vga_print_hex((uint32_t)(r->base + r->length - 1));
        vga_print_str("  ");
        ui_color(usable ? UI_GOOD : UI_DIM);
        vga_print_str(region_type(r->type));
        box_row_end();
    }
    box_end();
}
