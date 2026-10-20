#ifndef DEF_MMAP_PAGE_COUNT_H
#define DEF_MMAP_PAGE_COUNT_H

#include <stdint.h>

#define MMAP_MAX_BYTES (16 * 1024 * 1024)   // largest single mmap: 16 MiB

// Turn a byte length into whole pages. 1 = ok (*pages set), 0 = refused (logged).
int def_mmap_page_count(uint32_t length, uint32_t *pages);

#endif
