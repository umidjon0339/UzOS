/*
 * keyboard.c - Turn scancodes into characters and queue them.
 *
 * Scancode set 1:  press = key number,  release = key number | 0x80.
 * Arrow keys and friends first send 0xE0, then their key number.
 *
 * Uses: inb (arch/x86/io), register_interrupt_handler (arch/x86/isr),
 *       pic_unmask (pic/).
 */

#include "keyboard.h"
#include "isr.h"
#include "io.h"
#include "pic.h"

#define KEYBOARD_IRQ   1
#define KEYBOARD_DATA  0x60
#define RELEASED       0x80     // bit 7 of a scancode: key was let go
#define EXTENDED       0xE0     // next scancode is an "extended" key
#define QUEUE_SIZE     256

// Scancode -> character, US layout. 0 = no character (Ctrl, F1, ...).
static const char normal_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,                     // keypad *, Alt, Space, Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       // F1-F10
    0, 0,                               // Num Lock, Scroll Lock
    0, 0, 0, '-', 0, 0, 0, '+',         // keypad 7 8 9 - 4 5 6 +
};

static const char shift_map[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    0, 0, 0, '-', 0, 0, 0, '+',
};

// Scancodes of the modifier keys.
#define LEFT_SHIFT  0x2A
#define RIGHT_SHIFT 0x36
#define CAPS_LOCK   0x3A

static int shift_down;          // bit 0: left shift, bit 1: right shift
static int caps_lock;
static int extended;            // the last byte was 0xE0
static volatile char last_key;

// A ring buffer: the interrupt adds at 'head', the shell takes at 'tail'.
// head == tail means empty; one slot always stays unused, so a full queue
// (head + 1 == tail) can be told apart from an empty one.
static char queue[QUEUE_SIZE];
static volatile int head, tail;

static void enqueue(char c) {
    int next = (head + 1) % QUEUE_SIZE;
    if (next == tail)
        return;                 // full: drop the key rather than overwrite
    queue[head] = c;
    head = next;
    last_key = c;
}

char keyboard_pop_key(void) {
    if (head == tail)
        return 0;
    char c = queue[tail];
    tail = (tail + 1) % QUEUE_SIZE;
    return c;
}

static char extended_key(uint8_t scancode) {
    switch (scancode) {
        case 0x48: return (char)KEY_UP;
        case 0x50: return (char)KEY_DOWN;
        case 0x4B: return (char)KEY_LEFT;
        case 0x4D: return (char)KEY_RIGHT;
        case 0x47: return (char)KEY_HOME;
        case 0x4F: return (char)KEY_END;
        case 0x53: return (char)KEY_DELETE;
        case 0x49: return (char)KEY_PGUP;
        case 0x51: return (char)KEY_PGDN;
        default:   return 0;
    }
}

static char to_char(uint8_t scancode) {
    char plain   = normal_map[scancode];
    char shifted = shift_map[scancode];
    int  is_letter = plain >= 'a' && plain <= 'z';

    // Letters: Shift and Caps Lock cancel each other. Others: only Shift.
    int upper = is_letter ? ((shift_down != 0) ^ caps_lock) : (shift_down != 0);
    return (upper && shifted) ? shifted : plain;
}

static void keyboard_callback(registers_t *regs) {
    (void)regs;
    uint8_t scancode = inb(KEYBOARD_DATA);

    if (scancode == EXTENDED) {
        extended = 1;
        return;
    }
    if (extended) {
        extended = 0;
        if (!(scancode & RELEASED) && extended_key(scancode))
            enqueue(extended_key(scancode));
        return;
    }

    switch (scancode) {
        case LEFT_SHIFT:              shift_down |= 1;  return;
        case LEFT_SHIFT | RELEASED:   shift_down &= ~1; return;
        case RIGHT_SHIFT:             shift_down |= 2;  return;
        case RIGHT_SHIFT | RELEASED:  shift_down &= ~2; return;
        case CAPS_LOCK:               caps_lock = !caps_lock; return;
    }

    if (!(scancode & RELEASED) && to_char(scancode))
        enqueue(to_char(scancode));
}

void init_keyboard(void) {
    register_interrupt_handler(32 + KEYBOARD_IRQ, keyboard_callback);
    pic_unmask(KEYBOARD_IRQ);
}

char keyboard_get_last_key(void)  { return last_key; }
void keyboard_clear_last_key(void) { last_key = 0; }
