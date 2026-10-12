#ifndef KEYBOARD_H
#define KEYBOARD_H

// Special key codes (values > 127 to avoid ASCII conflicts)
#define KEY_UP      0x80
#define KEY_DOWN    0x81
#define KEY_LEFT    0x82
#define KEY_RIGHT   0x83
#define KEY_HOME    0x84
#define KEY_END     0x85
#define KEY_DELETE  0x86
#define KEY_PGUP    0x87
#define KEY_PGDN    0x88

void init_keyboard(void);
char keyboard_get_last_key(void);
void keyboard_clear_last_key(void);
char keyboard_pop_key(void);

#endif
