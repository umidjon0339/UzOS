/*
 * cmd_heapinfo.c - `heapinfo`: walk the kernel heap and summarise it.
 *
 * Uses: kheap_get_stats (core/kheap), box_* / ui_bar (ui module).
 */

#include "mem_commands.h"
#include "kheap.h"
#include "ui.h"
#include "box.h"

#define WIDTH 58

static void blocks_row(const char *label, uint32_t blocks, uint32_t bytes) {
    box_row();
    ui_label(label, 8);
    ui_printf("%5u blocks  %5u KB", blocks, bytes / 1024);
}

void cmd_heapinfo(const char *args) {
    (void)args;
    kheap_stats_t s;
    kheap_get_stats(&s);

    box_begin("Kernel heap", WIDTH);
    box_row();
    ui_label("Start", 8);
    vga_print_hex(KHEAP_START);
    ui_printf("   size %u KB", KHEAP_SIZE / 1024);
    box_row_end();

    box_row();
    ui_label("Usage", 8);
    ui_bar(s.bytes_used, s.bytes_used + s.bytes_free, 30);
    box_row_end();

    blocks_row("Used", s.blocks_used, s.bytes_used);
    box_row_end();
    blocks_row("Free", s.blocks_free, s.bytes_free);
    ui_color(UI_DIM);
    ui_printf("  (largest %u KB)", s.largest_free / 1024);
    box_row_end();

    box_row();
    ui_label("Health", 8);
    ui_color(s.corrupted ? UI_BAD : UI_GOOD);
    vga_print_str(s.corrupted ? GLYPH_CROSS " CORRUPTED: a block header was overwritten"
                              : GLYPH_CHECK " every block header is intact");
    box_row_end();
    box_end();
}
