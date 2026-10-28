#ifndef DEF_COARSE_TIMER_H
#define DEF_COARSE_TIMER_H

#include <stdint.h>

// A cache hit and a miss differ by only a couple of hundred cycles. A
// ring 3 attacker can tell them apart only with a clock at least that
// fine. 1 if a clock whose smallest step is 'resolution_cycles' is too
// COARSE to resolve a gap of 'gap_cycles' (so the attack is blind).
int def_too_coarse_to_resolve(uint32_t gap_cycles, uint32_t resolution_cycles);

#endif
