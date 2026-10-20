/*
 * cmd_play.c - `play`: a short melody on the PC speaker.
 *
 * A melody is just a list of (frequency, length) pairs played one after
 * the other, with a tiny gap so repeated notes sound separate.
 *
 * Uses: speaker_beep (speaker/), timer_wait (timer/), ui_* (ui module).
 */

#include "device_commands.h"
#include "speaker.h"
#include "timer.h"
#include "ui.h"

#define GAP_TICKS  5            // 50 ms of silence between notes

static const struct { uint32_t hz, ms; } melody[] = {
    { 262, 150 },   // C4
    { 330, 150 },   // E4
    { 392, 150 },   // G4
    { 523, 300 },   // C5
    { 392, 150 },   // G4
    { 523, 450 },   // C5
};

void cmd_play(const char *args) {
    (void)args;
    ui_color(UI_ACCENT);
    vga_print_str(GLYPH_NOTE " Playing the UzOS jingle");
    ui_end();

    for (unsigned i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
        speaker_beep(melody[i].hz, melody[i].ms);
        timer_wait(GAP_TICKS);
    }
}
