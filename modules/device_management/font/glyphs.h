#ifndef GLYPHS_H
#define GLYPHS_H

/*
 * glyphs.h - UzOS's own icons, drawn into the font.
 *
 * Character codes 1-31 are control codes in ASCII, but the VGA font
 * still has a picture for each (the old IBM smileys, hearts, notes...).
 * UzOS replaces some of them with icons. Print them like any text:
 *
 *     vga_print_str(GLYPH_FOLDER " docs");
 *
 * Codes 8, 9, 10 and 13 are left alone: they are '\b' '\t' '\n' '\r'.
 * So is 14, the musical note.
 * The values are octal escapes ("\001"), which never swallow the
 * characters after them the way "\x01d" would.
 */

#include <stdint.h>

#define GLYPH_FOLDER    "\001"      // directory
#define GLYPH_FILE      "\002"      // file
#define GLYPH_CHECK     "\003"      // success
#define GLYPH_CROSS     "\004"      // error
#define GLYPH_CLOCK     "\005"      // time
#define GLYPH_CHIP      "\006"      // CPU / memory
#define GLYPH_GEAR      "\007"      // process
#define GLYPH_LOCK      "\013"      // security
#define GLYPH_WARN      "\014"      // warning
#define GLYPH_STAR      "\017"      // star of the Uzbek flag
#define GLYPH_CRESCENT  "\022"      // crescent of the Uzbek flag
#define GLYPH_ARROW     "\023"      // list bullet
#define GLYPH_INFO      "\024"      // information

// Kept from the original font, used as they are:
#define GLYPH_NOTE      "\016"      // a musical note (beep, play)

// How many icons there are.
int glyphs_count(void);

// Icon number 'i': its 16 pixel rows go into 'rows'; returns its code.
uint8_t glyphs_get(int i, uint8_t rows[16]);

#endif
