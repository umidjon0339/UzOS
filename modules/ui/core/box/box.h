#ifndef BOX_H
#define BOX_H

/*
 * box.h - Frames, table cells and bars, drawn with box-drawing characters.
 *
 * The VGA font (code page 437) has line pieces: ┌ ─ ┐ │ └ ┘ ├ ┤ and
 * shades: █ ░. Put together they make panels:
 *
 *     box_begin("Physical memory", 40);    ┌─ Physical memory ────────────┐
 *     box_row();                           │ Used  ███░░░░░░░░   7%       │
 *     ui_bar(used, total, 12); ...         │                              │
 *     box_row_end();                       └──────────────────────────────┘
 *     box_end();
 *
 * Between box_row() and box_row_end() print at most width - 4
 * characters; box_row_end() fills the rest of the row with spaces.
 *
 * ui_cell() is how tables print text they did not write themselves
 * (process names, file names): it always takes exactly 'width' columns
 * and cannot be tricked into breaking the table (safe_text).
 */

#include <stdint.h>

void box_begin(const char *title, int width);
void box_row(void);
void box_row_end(void);
void box_divider(void);
void box_end(void);

// Exactly 'width' columns: cleaned, cut ("»" at the end) or padded.
void ui_cell(const char *text, int width);

// A number right-aligned in 'width' columns.
void ui_cell_int(int value, int width);

// A bar 'width' wide, filled part/whole: green, yellow above 60%, red above 85%.
void ui_bar(uint32_t part, uint32_t whole, int width);

#endif
