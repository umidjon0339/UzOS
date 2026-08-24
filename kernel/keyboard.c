#include "keyboard.h"
#include "io.h"
#include <stdint.h>

/* Scancode set 1 -> ASCII (US QWERTY, lowercase only)
 * Index = scancode, value = ASCII char (0 = unmapped) */
static const char scancode_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b', /* 0x00-0x0E */
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',    /* 0x0F-0x1C */
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',         /* 0x1D-0x29 */
    0,  '\\','z','x','c','v','b','n','m',',','.','/',0,'*',       /* 0x2A-0x37 */
    0,  ' '                                                        /* 0x38-0x39 */
};

/* Simple ring buffer for keyboard input */
#define KB_BUF_SIZE 64
static char kb_buffer[KB_BUF_SIZE];
static volatile int kb_head = 0;  /* write position */
static volatile int kb_tail = 0;  /* read position */

/* Called from IRQ1 ISR — read scancode, convert, buffer it */
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    /* Ignore key release events (bit 7 set) */
    if (scancode & 0x80) return;

    /* Only map known scancodes */
    if (scancode >= 128) return;

    char c = scancode_to_ascii[scancode];
    if (c == 0) return;  /* unmapped key */

    /* Push into ring buffer (drop if full) */
    int next = (kb_head + 1) % KB_BUF_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

/* Pop one character from buffer, return 0 if empty */
char keyboard_get_char(void) {
    if (kb_head == kb_tail) return 0;

    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    return c;
}
