#ifndef DEF_FILE_RANGE_H
#define DEF_FILE_RANGE_H

#include <stdint.h>

// Does [offset, offset + size) fit within 'limit' bytes? (logged if not)
int def_file_range_ok(uint32_t offset, uint32_t size, uint32_t limit);

#endif
