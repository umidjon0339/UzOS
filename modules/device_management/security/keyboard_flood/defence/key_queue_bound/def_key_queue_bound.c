/*
 * def_key_queue_bound.c - DEFENCE key_queue_bound: a full key ring drops.
 *
 * Stops:  keyboard_flood (security/keyboard_flood)
 * Called: enqueue() (keyboard/keyboard.c), for every key the IRQ adds.
 * Rule:   room exists only if (head + 1) % size != tail; a full ring
 *         drops the newest key instead of overwriting an old one.
 * Why:    the keyboard interrupt fires faster than the shell reads. A
 *         fixed ring buffer that drops when full can never overrun into
 *         kernel memory, however fast keys arrive. (No audit log here:
 *         this runs in the interrupt, where logging is unsafe.)
 */

#include "def_key_queue_bound.h"
#include "registry.h"

// The whole rule, as a pure function so the selftest can try it.
static int has_room(int head, int tail, int size) {
    return ((head + 1) % size) != tail;
}

int def_key_queue_has_room(int head, int tail, int size) {
    return has_room(head, tail, size);
}

static int selftest(void) {
    int n = KEY_QUEUE_SIZE;
    if (!has_room(0, 0, n)) return 0;          // empty: room
    if (!has_room(5, 0, n)) return 0;          // partly full: room
    if (has_room(n - 1, 0, n)) return 0;       // head+1 == tail(0): full
    if (has_room(4, 5, n)) return 0;           // full mid-buffer
    if (!has_room(n - 1, n - 1, n)) return 0;  // empty at the top (wrap): room
    return 1;
}

static const defence_t defence = {
    "key_queue_bound",
    "the key ring buffer drops input when full; it never overruns",
    "keyboard_flood",
    selftest
};

void def_key_queue_bound_register(void) { registry_add_defence(&defence); }
