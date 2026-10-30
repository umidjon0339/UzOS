#ifndef DEF_REASSEMBLY_BOUNDS_H
#define DEF_REASSEMBLY_BOUNDS_H

/*
 * def_reassembly_bounds.h - DEFENCE reassembly_bounds: a fragment must end inside the reassembly buffer
 */

#include <stdint.h>

// May a fragment of 'len' bytes at byte 'offset' be copied into a
// buffer of 'size' bytes? ipv4_reassemble() asks. 1 = yes, 0 = no (logged).
int def_reasm_fits(uint32_t offset, uint32_t len, uint32_t size);

#endif
