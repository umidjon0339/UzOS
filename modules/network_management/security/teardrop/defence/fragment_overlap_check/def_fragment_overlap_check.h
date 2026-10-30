#ifndef DEF_FRAGMENT_OVERLAP_CHECK_H
#define DEF_FRAGMENT_OVERLAP_CHECK_H

/*
 * def_fragment_overlap_check.h - DEFENCE fragment_overlap_check: a datagram whose fragments overlap is dropped whole
 */

#include <stdint.h>

// 'filled' (bit per 8-byte block) and a new fragment covering blocks
// first .. last: may it go in? ipv4_reassemble() asks. 1 = no overlap,
// 0 = overlap: drop the datagram (logged).
int def_frag_no_overlap(const uint8_t *filled, uint32_t first, uint32_t last);

#endif
