#ifndef DEF_KEY_QUEUE_BOUND_H
#define DEF_KEY_QUEUE_BOUND_H

// The keyboard ring buffer has this many slots (one always kept unused,
// so "full" and "empty" can be told apart).
#define KEY_QUEUE_SIZE 256

// Is there room to add one more key, given 'head', 'tail' and 'size'?
// enqueue() in keyboard.c asks this for every key: when the ring is
// full the newest key is dropped, so a flood of scancodes can never
// overrun the buffer or kernel memory. 1 = room, 0 = full (drop).
int def_key_queue_has_room(int head, int tail, int size);

#endif
