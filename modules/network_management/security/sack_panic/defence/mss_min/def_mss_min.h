#ifndef DEF_MSS_MIN_H
#define DEF_MSS_MIN_H

/*
 * def_mss_min.h - DEFENCE mss_min: a peer's MSS below 48 is raised to 48
 */

#include <stdint.h>

#define TCP_MSS_MIN 48

// The MSS to use when the peer announced 'mss'. tcp.c asks.
uint16_t def_mss_clamp(uint16_t mss);

#endif
