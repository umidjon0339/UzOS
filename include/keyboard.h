#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Called by IRQ1 ISR — reads scancode and buffers it */
void keyboard_handler(void);

/* Get next typed character from buffer (0 if empty) */
char keyboard_get_char(void);

#endif
