#ifndef SYS_GETCAPS_H
#define SYS_GETCAPS_H

/*
 * sys_getcaps.h - getcaps(): which privileges do I have?
 *
 * The answer is a bit set: bit CAP_KILL, bit CAP_SETUID, ... (core/caps).
 * A program can test a bit before it tries something, instead of trying
 * and being refused.
 */

#include <stdint.h>

#define SYS_GETCAPS 25  // int 0x80 with EAX = 25; the bit set comes back in EAX

uint32_t sys_getcaps(void);

#endif
