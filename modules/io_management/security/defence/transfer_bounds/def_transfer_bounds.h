#ifndef DEF_TRANSFER_BOUNDS_H
#define DEF_TRANSFER_BOUNDS_H

#include <stdint.h>

// May read()/write() use 'count' bytes at 'buf'? 1 = yes, 0 = refused (logged).
int def_transfer_ok(const void *buf, uint32_t count);

#endif
