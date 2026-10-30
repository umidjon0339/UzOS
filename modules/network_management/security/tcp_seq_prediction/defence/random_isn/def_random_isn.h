#ifndef DEF_RANDOM_ISN_H
#define DEF_RANDOM_ISN_H

/*
 * def_random_isn.h - DEFENCE random_isn: every connection's ISN is a keyed hash of its addresses plus a clock
 */

#include <stdint.h>

// The initial sequence number for a connection (host-order addresses).
uint32_t def_isn(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);

#endif
