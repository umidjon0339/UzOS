#ifndef DEF_HEAP_CANARY_H
#define DEF_HEAP_CANARY_H

#include "kheap.h"

// Is this block header intact, so it is safe to follow? 1 = yes, 0 = no (logged).
int def_heap_block_ok(const kheap_header_t *h);

#endif
