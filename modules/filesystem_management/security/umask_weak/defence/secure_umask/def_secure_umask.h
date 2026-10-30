#ifndef DEF_SECURE_UMASK_H
#define DEF_SECURE_UMASK_H

#include <stdint.h>

#define MODE_CREATE_BASE 066    // what a new file or directory asks for: rw/rw

// The mode a NEW file or directory gets: 'base' without the bits in the
// creator's 'umask' - and never "others may write", whatever the umask
// says (only an explicit chmod gives that). ramdisk_create() asks this.
uint32_t def_creation_mode(uint32_t base, uint32_t umask);

#endif
