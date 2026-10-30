#ifndef DEF_NLINK_BOUNDS_H
#define DEF_NLINK_BOUNDS_H

#include <stdint.h>

#define FS_LINK_MAX 16          // names one file may have (Linux ext4: 65000)

// May a file that already has 'nlink' names get one more?
// ramdisk_link() asks this. 1 = allowed, 0 = refused (logged): EMLINK.
int def_nlink_ok(uint32_t nlink);

#endif
