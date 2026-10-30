#ifndef DEF_MARTIAN_FILTER_H
#define DEF_MARTIAN_FILTER_H

/*
 * def_martian_filter.h - DEFENCE martian_filter: packets from impossible sources (0.0.0.0, broadcast, multicast, ourselves) are dropped
 */

#include <stdint.h>

// Is 'src' a possible source for a packet to 'dst'? ipv4_receive()
// asks. 1 = yes, 0 = a martian: drop (logged).
int def_martian_ok(uint32_t src, uint32_t dst);

#endif
