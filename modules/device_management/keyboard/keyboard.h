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
#define KEY_CTRL_UP   0x89  // Ctrl+Up:   a bigger screen (vga/modes)
#define KEY_CTRL_DOWN 0x8A  // Ctrl+Down: a smaller screen
#define KEY_ESCAPE  27
#define KEY_CTRL_C  3       // ASCII 3, "end of text": what Ctrl+C sends on every terminal
#define KEY_CTRL_Z  26      // ASCII 26, "substitute": Ctrl+Z, stop the foreground job

void init_keyboard(void);

// Take the oldest typed key from the queue, or 0 if it is empty.
char keyboard_pop_key(void);

// Ctrl+C is special: it must work even while nobody reads the queue
// (a command is running). The shell installs a handler; the driver calls
// it from the interrupt. If it returns 1 it took care of Ctrl+C;
// otherwise KEY_CTRL_C is queued like a normal key.
typedef int (*keyboard_ctrl_c_handler_t)(void);
void keyboard_on_ctrl_c(keyboard_ctrl_c_handler_t handler);

// Ctrl+Z the same way: the shell stops the foreground job (SIGTSTP).
void keyboard_on_ctrl_z(keyboard_ctrl_c_handler_t handler);

// The most recent key pressed (it stays until cleared). Used by games.
char keyboard_get_last_key(void);
void keyboard_clear_last_key(void);

#endif
