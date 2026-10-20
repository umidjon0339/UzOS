/*
 * vga_text.c - The two text-mode displays: 80x25 and 80x50.
 *
 * Opening a text display programs the whole card from the tables below
 * (graphics mode may have changed anything), loads the font into plane
 * 2, and loads the colour theme into the DAC.
 *
 *   80x25: 400 pixel lines / 16 rows per character = 25 lines of text
 *   80x50: 400 pixel lines /  8 rows per character = 50 lines of text
 *
 * Characters are 8 pixels wide (the BIOS uses 9), so 80 columns are
 * exactly 640 pixels and block characters like the bars touch.
 *
 * The first time a text display opens, the card still has the BIOS
 * font: it is read out and handed to font/ (font_set_bios).
 *
 * Uses: outb/inb (arch/x86/io), font_glyph / font_set_bios (font/),
 *       palette_apply_vga (vga/palette).
 */

#include "vga_text.h"
#include "io.h"
#include "font.h"
#include "palette.h"

#define COLS            80
#define TEXT_MEMORY     ((volatile uint16_t *)0xB8000)

#define MISC_WRITE      0x3C2   // VGA register groups: (index port, index port + 1)
#define SEQ_INDEX       0x3C4
#define GC_INDEX        0x3CE
#define CRTC_INDEX      0x3D4
#define AC_INDEX        0x3C0   // attribute controller: index and data on ONE port
#define INPUT_STATUS    0x3DA   // reading it makes the next AC write an index

#define CRTC_CHAR_HEIGHT  0x09  // low 5 bits: pixel rows per character - 1
#define CRTC_CURSOR_START 0x0A  // bit 5 = cursor hidden
#define CRTC_CURSOR_END   0x0B
#define CRTC_CURSOR_HIGH  0x0E
#define CRTC_CURSOR_LOW   0x0F

#define FONT_MEMORY     ((volatile uint8_t *)0xA0000)  // plane 2, once opened
#define FONT_SLOT       32      // bytes per character in plane 2 (up to 32 rows)

// Standard VGA text mode 3, with 8-pixel-wide characters.
static const uint8_t misc = 0x63;                       // 25 MHz dot clock
static const uint8_t seq[5] = { 0x03, 0x01, 0x03, 0x00, 0x02 };   // [1] = 8-dot chars
static const uint8_t crtc[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0E, 0x0F,
    0x00, 0x00, 0x00, 0x50, 0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF,
};
static const uint8_t gc[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF };
static const uint8_t ac[21] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,     // colour i -> DAC entry i
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x04,       // mode: text, NO blinking, so backgrounds can use all 16 colours
    0x00,       // border colour: 0, the theme's background
    0x0F, 0x00, 0x00,
};

static int char_height = 16;    // 16 (80x25) or 8 (80x50)

/* ---------- Registers ---------- */

static void write_reg(uint16_t index_port, uint8_t index, uint8_t value) {
    outb(index_port, index);
    outb(index_port + 1, value);
}

static void set_mode_registers(void) {
    outb(MISC_WRITE, misc);
    for (int i = 0; i < 5; i++)
        write_reg(SEQ_INDEX, i, seq[i]);

    // CRTC registers 0-7 are write-protected by bit 7 of register 0x11.
    write_reg(CRTC_INDEX, 0x11, crtc[0x11] & 0x7F);
    for (int i = 0; i < 25; i++)
        write_reg(CRTC_INDEX, i, i == 0x11 ? (crtc[i] & 0x7F) : crtc[i]);

    // The only difference between 80x25 and 80x50: rows per character,
    // and where in the character the underline cursor sits.
    write_reg(CRTC_INDEX, CRTC_CHAR_HEIGHT, (crtc[CRTC_CHAR_HEIGHT] & 0xE0) | (char_height - 1));
    write_reg(CRTC_INDEX, CRTC_CURSOR_START, char_height - 2);
    write_reg(CRTC_INDEX, CRTC_CURSOR_END, char_height - 1);

    for (int i = 0; i < 9; i++)
        write_reg(GC_INDEX, i, gc[i]);

    for (int i = 0; i < 21; i++) {
        inb(INPUT_STATUS);              // next AC write is an index
        outb(AC_INDEX, i);
        outb(AC_INDEX, ac[i]);
    }
    inb(INPUT_STATUS);
    outb(AC_INDEX, 0x20);               // done: let the card show the screen again
}

/* ---------- The font in plane 2 ---------- */

// In text mode the font lives in memory plane 2, which the CPU cannot
// normally see. Switch the card so plane 2 appears flat at 0xA0000...
static void open_font_plane(void) {
    write_reg(SEQ_INDEX, 2, 0x04);      // write only to plane 2
    write_reg(SEQ_INDEX, 4, 0x07);      // flat addressing (no odd/even)
    write_reg(GC_INDEX, 4, 0x02);       // read from plane 2
    write_reg(GC_INDEX, 5, 0x00);       // no odd/even
    write_reg(GC_INDEX, 6, 0x04);       // map the planes at 0xA0000
}

// ...and back to normal text mode access at 0xB8000.
static void close_font_plane(void) {
    write_reg(SEQ_INDEX, 2, 0x03);      // write to planes 0 and 1 (chars, colours)
    write_reg(SEQ_INDEX, 4, 0x03);      // odd/even addressing
    write_reg(GC_INDEX, 4, 0x00);
    write_reg(GC_INDEX, 5, 0x10);
    write_reg(GC_INDEX, 6, 0x0E);       // map text memory at 0xB8000
}

// The first time only: keep the BIOS font before we overwrite it.
static void save_bios_font(void) {
    static int saved;
    static uint8_t glyphs[256][16];
    if (saved)
        return;

    open_font_plane();
    for (int c = 0; c < 256; c++)
        for (int r = 0; r < 16; r++)
            glyphs[c][r] = FONT_MEMORY[c * FONT_SLOT + r];
    close_font_plane();

    font_set_bios(glyphs);
    saved = 1;
}

static void load_font(void) {
    open_font_plane();
    for (int c = 0; c < 256; c++) {
        const uint8_t *glyph = font_glyph(c, char_height);
        for (int r = 0; r < FONT_SLOT; r++)
            FONT_MEMORY[c * FONT_SLOT + r] = r < char_height ? glyph[r] : 0;
    }
    close_font_plane();
}

/* ---------- The display functions ---------- */

static int open_layout(int height) {
    save_bios_font();
    char_height = height;
    set_mode_registers();
    load_font();
    palette_apply_vga();
    return 1;
}

static int open_25(void) { return open_layout(16); }
static int open_50(void) { return open_layout(8); }

static void close_text(void) {
    // Nothing to undo: the next display programs the card itself.
}

static void put(int x, int y, uint16_t cell) {
    TEXT_MEMORY[y * COLS + x] = cell;
}

static void cursor(int x, int y, int visible) {
    if (!visible) {
        write_reg(CRTC_INDEX, CRTC_CURSOR_START, 0x20);     // bit 5: hidden
        return;
    }
    uint16_t pos = y * COLS + x;
    write_reg(CRTC_INDEX, CRTC_CURSOR_START, char_height - 2);
    write_reg(CRTC_INDEX, CRTC_CURSOR_LOW, pos & 0xFF);
    write_reg(CRTC_INDEX, CRTC_CURSOR_HIGH, pos >> 8);
}

static void reload(void) {
    load_font();
    palette_apply_vga();
}

const display_t vga_text_80x25 = {
    "80x25", "text mode, 16-pixel font (the classic PC screen)",
    80, 25, open_25, close_text, put, cursor, reload
};

const display_t vga_text_80x50 = {
    "80x50", "text mode, 8-pixel font: twice the lines",
    80, 50, open_50, close_text, put, cursor, reload
};
