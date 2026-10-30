/*
 * modes.c - The list of displays, smallest first, and stepping through it.
 *
 * Uses: the displays (vga/text, gui/fbcon), vga_set_display (vga/),
 *       strcmp (lib/string).
 */

#include "modules/device_management/vga/modes/modes.h"
#include "modules/device_management/vga/vga.h"              // vga_display, vga_set_display
#include "modules/device_management/vga/text/vga_text.h"    // vga_text_80x25, _80x50
#include "modules/device_management/gui/fbcon/fbcon.h"      // fbcon_800x600 ... _1920x1080
#include "lib/string/string.h"                              // strcmp

static const display_t *const modes[] = {
    &vga_text_80x25, &vga_text_80x50,
    &fbcon_800x600, &fbcon_1024x768, &fbcon_1280x720, &fbcon_1600x900, &fbcon_1920x1080,
};
#define MODE_COUNT ((int)(sizeof(modes) / sizeof(modes[0])))

int modes_count(void) {
    return MODE_COUNT;
}

const display_t *modes_at(int index) {
    if (index < 0 || index >= MODE_COUNT)
        return 0;
    return modes[index];
}

const display_t *modes_find(const char *name) {
    for (int i = 0; i < MODE_COUNT; i++)
        if (strcmp(name, modes[i]->name) == 0)
            return modes[i];
    return 0;
}

const display_t *modes_next(const display_t *now, int step, int graphics_only) {
    int i = 0;
    while (i < MODE_COUNT && modes[i] != now)
        i++;
    if (i == MODE_COUNT)
        return 0;                           // 'now' is not in the list

    // Walk in the 'step' direction; skip text modes if asked to.
    for (i += step; i >= 0 && i < MODE_COUNT; i += step)
        if (!graphics_only || modes[i]->width)
            return modes[i];
    return 0;
}

int modes_step(int step) {
    const display_t *next = modes_next(vga_display(), step, 0);
    return next && vga_set_display(next);
}
