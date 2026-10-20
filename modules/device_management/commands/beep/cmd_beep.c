/*
 * cmd_beep.c - `beep [hz]`: a short tone (440 Hz, the note A, by default).
 *
 * Uses: speaker_beep (speaker/), atoi (lib/stdlib), ui_* (ui module).
 */

#include "device_commands.h"
#include "speaker.h"
#include "stdlib.h"
#include "ui.h"

#define DEFAULT_HZ  440
#define DURATION_MS 200

void cmd_beep(const char *args) {
    int hz = args[0] ? atoi(args) : DEFAULT_HZ;
    if (hz < SPEAKER_MIN_HZ || hz > SPEAKER_MAX_HZ) {
        ui_error("beep", "frequency must be 20 - 20000 Hz");
        return;
    }

    ui_color(UI_ACCENT);
    ui_printf(GLYPH_NOTE " %d Hz", hz);
    ui_end();

    speaker_beep(hz, DURATION_MS);
}
