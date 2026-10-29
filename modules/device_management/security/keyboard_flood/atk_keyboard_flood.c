/*
 * atk_keyboard_flood.c - ATTACK keyboard_flood: type faster than reads.
 *
 * Breaks:     resource - the key buffer is finite and must not overrun
 * Stopped by: key_queue_bound (defence/key_queue_bound)
 *
 * The keyboard interrupt adds a key on every press; the shell takes
 * them out later. If keys arrive faster than they are read, a buffer
 * with no limit would overrun into whatever follows it in memory. UzOS
 * uses a fixed ring buffer that DROPS the newest key when full, so
 * hammering the keyboard loses keystrokes but never corrupts memory.
 *
 * simulate(): explain the ring buffer and the one-slot-free trick.
 * launch():   hammer the real rule with 4x the buffer's capacity and
 *             show it stores at most size-1 and drops the rest.
 * cleanup():  nothing (launch only exercises the rule).
 *
 * Uses: key_queue_bound (its defence), vga_print_* (vga).
 */

#include "registry.h"
#include "def_key_queue_bound.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  the key buffer has KEY_QUEUE_SIZE slots (one kept free)\n");
    vga_print_str("  keys arriving past that are dropped, not stored\n");
    vga_print_str("  so typing fast can never overrun kernel memory\n");
    return 1;
}

static int launch(void) {
    int head = 0, tail = 0, stored = 0, dropped = 0;
    int flood = KEY_QUEUE_SIZE * 4;

    for (int i = 0; i < flood; i++) {
        if (def_key_queue_has_room(head, tail, KEY_QUEUE_SIZE)) {
            head = (head + 1) % KEY_QUEUE_SIZE;
            stored++;
        } else {
            dropped++;
        }
    }
    vga_print_str("  flooded "); vga_print_int(flood);
    vga_print_str(" keys: stored "); vga_print_int(stored);
    vga_print_str(", dropped "); vga_print_int(dropped);
    vga_print_str("\n");
    return stored > KEY_QUEUE_SIZE - 1 ? stored - (KEY_QUEUE_SIZE - 1) : 0;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "keyboard_flood",
    "type faster than the shell reads to overrun the key buffer",
    INV_RESOURCE, SEV_LOW,
    simulate, launch, cleanup,
    "key_queue_bound"
};

void atk_keyboard_flood_register(void) { registry_add_attack(&attack); }
