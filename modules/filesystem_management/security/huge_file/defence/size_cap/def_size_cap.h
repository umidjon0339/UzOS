#ifndef DEF_SIZE_CAP_H
#define DEF_SIZE_CAP_H

#include <stdint.h>

// May the calling process make a file 'size' bytes long (truncate,
// ftruncate, fallocate)? Not past RAMDISK_MAX_FILE_SIZE, and not past
// its RLIMIT_FSIZE. 1 = allowed, 0 = refused (logged).
int def_new_size_ok(uint32_t size);

#endif
