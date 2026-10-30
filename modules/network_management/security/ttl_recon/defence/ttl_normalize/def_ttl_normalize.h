#ifndef DEF_TTL_NORMALIZE_H
#define DEF_TTL_NORMALIZE_H

/*
 * def_ttl_normalize.h - DEFENCE ttl_normalize (ILLUSTRATION): the default TTL is the common 64
 */

#include <stdint.h>

#define TTL_COMMON 64

// The TTL for a packet whose sender did not choose one.
uint8_t def_default_ttl(void);

#endif
