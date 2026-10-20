#ifndef DEF_USER_PTR_BOUNDS_H
#define DEF_USER_PTR_BOUNDS_H

#include <stdint.h>

// Is [start, start + size) the caller's own memory: inside user space,
// not wrapping, and every page mapped for ring 3? 1 = yes, 0 = refused (logged).
int def_user_range_ok(uint32_t start, uint32_t size);

// Is there a NUL-terminated string at 'start', at most 'max' bytes long,
// entirely in mapped user memory? 1 = yes, 0 = refused (logged).
int def_user_string_ok(uint32_t start, uint32_t max);

#endif
