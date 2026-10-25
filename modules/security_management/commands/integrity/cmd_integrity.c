/*
 * cmd_integrity.c - `integrity`: is the kernel still the one that booted?
 *
 *   > Kernel integrity
 *     REGION     SECTION  SIZE     AT BOOT    NOW
 *     code       .text    170 KB   4f1c09a2   4f1c09a2  unchanged
 *     constants  .rodata   25 KB   9be0712c   9be0712c  unchanged
 *
 * Shows the first 4 bytes of each SHA-256; they are compared in full.
 *
 * Uses: integrity_* (core/integrity), tsc_* (cpu_management) to time
 *       the check, ui_* / ui_cell (ui module).
 */

#include "security_commands.h"
#include "integrity.h"
#include "tsc.h"
#include "ui.h"
#include "box.h"

#define HASH_SHOWN 4            // bytes of each hash to print

static void print_hash(const uint8_t *hash) {
    for (int i = 0; i < HASH_SHOWN; i++)
        ui_printf("%02x", hash[i]);
    vga_print_str("   ");
}

void cmd_integrity(const char *args) {
    (void)args;
    uint8_t now[SHA256_SIZE];
    int changed = 0;

    ui_title("Kernel integrity");
    ui_color(UI_TITLE);
    vga_print_str("  REGION     SECTION  SIZE     AT BOOT    NOW\n");
    ui_reset();

    uint64_t start = tsc_now();
    for (int i = 0; i < integrity_region_count(); i++) {
        const integrity_region_t *r = integrity_region(i);
        int same = integrity_verify_region(i, now);
        changed += !same;

        vga_print_str("  ");
        ui_cell(r->name, 11);
        ui_cell(r->section, 9);
        ui_printf("%3u KB   ", (uint32_t)(r->end - r->start) / 1024);
        ui_color(UI_DIM);
        print_hash(r->at_boot);
        ui_color(same ? UI_DIM : UI_BAD);
        print_hash(now);
        ui_color(same ? UI_GOOD : UI_BAD);
        vga_print_str(same ? "unchanged" : "CHANGED");
        ui_end();
    }
    uint32_t us = tsc_cycles_to_us(tsc_now() - start);

    if (changed)
        ui_error("integrity", "the kernel was written to since boot: exec refuses programs");
    else
        ui_ok("integrity", "the kernel is the one that booted");
    ui_color(UI_DIM);
    ui_printf("  measured with SHA-256 in %u.%u ms; exec measures it again every time",
              us / 1000, us % 1000 / 100);
    ui_end();
}
