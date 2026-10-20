#ifndef DEF_MOUSE_PACKET_SYNC_H
#define DEF_MOUSE_PACKET_SYNC_H

#include <stdint.h>

// Can 'byte' be the first byte of a packet? 1 = yes, 0 = dropped (logged).
int def_mouse_sync_ok(uint8_t byte);

// Is a complete packet usable (no overflowed movement)? 1 = yes, 0 = dropped (logged).
int def_mouse_packet_ok(const uint8_t *packet);

#endif
