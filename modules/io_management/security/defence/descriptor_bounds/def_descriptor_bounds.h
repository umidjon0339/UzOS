#ifndef DEF_DESCRIPTOR_BOUNDS_H
#define DEF_DESCRIPTOR_BOUNDS_H

#include <stdint.h>
#include "process.h"

// May process p use descriptor 'fd' for 'need' (O_READ, O_WRITE, or 0 =
// "any open file")? 1 = yes, 0 = refused (logged).
int def_fd_ok(const process_t *p, int fd, uint32_t need);

#endif
