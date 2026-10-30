#ifndef MODES_H
#define MODES_H

/*
 * modes.h - Every display UzOS has, from the smallest to the biggest.
 *
 *   80x25  80x50  |  800x600  1024x768  1280x720  1600x900  1920x1080
 *   text modes    |  graphics modes (gui/fbcon)
 *
 * `mode <name>` picks one by name. Ctrl+Up / Ctrl+Down step to the next
 * bigger / smaller one; QEMU's window grows or shrinks to match.
 */

#include "modules/device_management/vga/display.h"     // display_t

int modes_count(void);
const display_t *modes_at(int index);

// The display called 'name' ("1280x720"), or 0 if there is none.
const display_t *modes_find(const char *name);

// The display one step bigger (step = +1) or smaller (step = -1) than
// 'now', or 0 at either end. graphics_only skips the text modes (the
// desktop can only use graphics).
const display_t *modes_next(const display_t *now, int step, int graphics_only);

// Switch the console one step bigger / smaller. Returns 1 if it changed.
int modes_step(int step);

#endif
