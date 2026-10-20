#ifndef KEYBOARD_H
#define KEYBOARD_H

/*
 * keyboard.h - PS/2 keyboard, IRQ 1.
 *
 * Each key press or release sends a "scancode" (a key number, not a
 * letter) to port 0x60. The driver turns presses into characters and
 * keeps them in a small queue until the shell reads them.
 */

// Special keys, outside ASCII. Compare as (unsigned char)c == KEY_UP.
#define KEY_UP      0x80
#define KEY_DOWN    0x81
#define KEY_LEFT    0x82
#define KEY_RIGHT   0x83
#define KEY_HOME    0x84
#define KEY_END     0x85
#define KEY_DELETE  0x86
#define KEY_PGUP    0x87
#define KEY_PGDN    0x88
#define KEY_ESCAPE  27

void init_keyboard(void);

// Take the oldest typed key from the queue, or 0 if it is empty.
char keyboard_pop_key(void);

// The most recent key pressed (it stays until cleared). Used by games.
char keyboard_get_last_key(void);
void keyboard_clear_last_key(void);

#endif
