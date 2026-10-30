#ifndef DEF_FRAG_MEM_LIMIT_H
#define DEF_FRAG_MEM_LIMIT_H

/*
 * def_frag_mem_limit.h - DEFENCE frag_mem_limit: one source may hold at most 2 of the 4 reassembly slots
 */

#include <stdint.h>

#define FRAG_SOURCE_MAX 2       // reassembly slots one source may hold

// May a source that holds 'held' slots start another? ipv4_reassemble()
// asks. 1 = yes, 0 = no (logged).
int def_frag_slot_ok(int held);

#endif
