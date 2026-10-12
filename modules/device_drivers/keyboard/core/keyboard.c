#include "keyboard.h"
#include "isr.h"
#include "io.h"
#include "shell.h"

static volatile char last_key = 0;
static volatile int e0_prefix = 0; // Track extended scancode prefix
static volatile int shift_pressed = 0; // Bit 0: Left shift, Bit 1: Right shift
static volatile int caps_lock_active = 0; // Caps Lock state

// Basic US layout scancode mapping (unshifted)
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

// Shifted US layout scancode mapping
unsigned char kbdus_shift[128] =
{
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
  '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_buf_head = 0;
static int kbd_buf_tail = 0;

static void kbd_push(char c) {
    int next_head = (kbd_buf_head + 1) % KBD_BUFFER_SIZE;
    if (next_head != kbd_buf_tail) {
        kbd_buffer[kbd_buf_head] = c;
        kbd_buf_head = next_head;
    }
}

static void keyboard_callback(registers_t *regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);
    
    // Handle 0xE0 prefix for extended keys (arrows, home, end, etc.)
    if (scancode == 0xE0) {
        e0_prefix = 1;
        return;
    }
    
    if (e0_prefix) {
        e0_prefix = 0;
        // Only handle key press, not release (bit 7 set = release)
        if (!(scancode & 0x80)) {
            char special = 0;
            switch (scancode) {
                case 0x48: special = KEY_UP;     break;
                case 0x50: special = KEY_DOWN;   break;
                case 0x4B: special = KEY_LEFT;   break;
                case 0x4D: special = KEY_RIGHT;  break;
                case 0x47: special = KEY_HOME;   break;
                case 0x4F: special = KEY_END;    break;
                case 0x53: special = KEY_DELETE; break;
                case 0x49: special = KEY_PGUP;   break;
                case 0x51: special = KEY_PGDN;   break;
            }
            if (special) {
                last_key = special;
                kbd_push(special);
            }
        }
        return;
    }

    // Handle modifier state keys (Shift press/release, Capslock press)
    if (scancode == 0x2A) { shift_pressed |= 1;  return; } // Left Shift press
    if (scancode == 0xAA) { shift_pressed &= ~1; return; } // Left Shift release
    if (scancode == 0x36) { shift_pressed |= 2;  return; } // Right Shift press
    if (scancode == 0xB6) { shift_pressed &= ~2; return; } // Right Shift release
    if (scancode == 0x3A) { caps_lock_active = !caps_lock_active; return; } // Caps Lock toggle
    
    // Handle key presses (scancodes < 0x80)
    if (!(scancode & 0x80)) {
        char unshifted = kbdus[scancode];
        char shifted   = kbdus_shift[scancode];
        char c = 0;

        if (unshifted >= 'a' && unshifted <= 'z') {
            // Letter key: Shift XOR Caps Lock determines uppercase
            if ((shift_pressed != 0) ^ caps_lock_active) {
                c = shifted;
            } else {
                c = unshifted;
            }
        } else {
            // Non-letter key: Shift determines shifted character
            if (shift_pressed != 0) {
                c = shifted ? shifted : unshifted;
            } else {
                c = unshifted;
            }
        }

        if (c != 0) {
            last_key = c;
            kbd_push(c);
        }
    }
}

char keyboard_pop_key(void) {
    if (kbd_buf_head == kbd_buf_tail) return 0;
    
    char c = kbd_buffer[kbd_buf_tail];
    kbd_buf_tail = (kbd_buf_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

void init_keyboard(void) {
    // IRQ1 is mapped to interrupt vector 33
    register_interrupt_handler(33, keyboard_callback);
}

char keyboard_get_last_key(void) {
    return last_key;
}

void keyboard_clear_last_key(void) {
    last_key = 0;
}
