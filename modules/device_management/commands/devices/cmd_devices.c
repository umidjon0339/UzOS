/*
 * cmd_devices.c - `devices`: what hardware there is and how busy it is.
 *
 * For each IRQ line in use: its interrupt vector, whether the PIC lets it
 * through, whether a handler is registered, and how often it fired.
 * Press some keys and run it again: the keyboard count goes up.
 *
 * Uses: isr_irq_count / isr_has_handler (arch/x86/isr), pic_get_mask
 *       (pic/), timer_frequency, mouse_is_available, bga_is_available,
 *       rtc_read, vga_display (vga/), box_* (ui module).
 */

#include "device_commands.h"
#include "isr.h"
#include "pic.h"
#include "timer.h"
#include "mouse.h"
#include "bga.h"
#include "rtc.h"
#include "format.h"
#include "ui.h"
#include "box.h"

#define WIDTH 64

static const struct {
    uint8_t irq;
    const char *name;
} lines[] = {
    {  0, "PIT timer" },
    {  1, "PS/2 keyboard" },
    {  2, "cascade (PIC2)" },
    { 12, "PS/2 mouse" },
};

static void found_row(const char *label, int found, const char *detail) {
    box_row();
    ui_label(label, 12);
    ui_color(found ? UI_GOOD : UI_BAD);
    vga_print_str(found ? GLYPH_CHECK " " : GLYPH_CROSS " ");
    ui_reset();
    vga_print_str(detail);
    box_row_end();
}

void cmd_devices(const char *args) {
    (void)args;
    uint16_t mask = pic_get_mask();
    char text[48];

    box_begin("Interrupt lines", WIDTH);
    box_row();
    ui_color(UI_TITLE);
    ui_printf("IRQ vector  %-15s %-7s handler  interrupts", "device", "PIC");
    box_row_end();
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        uint8_t irq = lines[i].irq;
        int open = !(mask & (1 << irq));
        box_row();
        ui_printf("%3d  %4d   %-15s ", irq, IRQ_BASE + irq, lines[i].name);
        ui_color(open ? UI_GOOD : UI_DIM);
        ui_printf("%-7s ", open ? "open" : "masked");
        ui_reset();
        ui_printf("%-7s  %10u",
                  irq == PIC_CASCADE_IRQ ? "-" : isr_has_handler(IRQ_BASE + irq) ? "yes" : "no",
                  isr_irq_count(irq));
        box_row_end();
    }
    box_row();
    ui_color(UI_DIM);
    ui_printf("spurious IRQs ignored: %u", isr_spurious_count());
    box_row_end();

    box_divider();
    format(text, sizeof(text), "%u Hz", timer_frequency());
    found_row("Timer", 1, text);
    rtc_time_t now;
    found_row("Clock (RTC)", rtc_read(&now), "real-time clock, `date`");
    found_row("Mouse", mouse_is_available(), "PS/2, wheel scrolls the console");
    found_row("BGA video", bga_is_available(), "graphics: startx, mode 1280x720");
    format(text, sizeof(text), "%s", vga_display()->name);
    found_row("Display", 1, text);
    box_end();
}
