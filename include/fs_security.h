#ifndef FS_SECURITY_H
#define FS_SECURITY_H

#include <stdint.h>

#define FS_NAME_MAX 127

int fs_security_check_path(const char *path);
int fs_security_file_range_valid(uint32_t offset, uint32_t count,
                                 uint32_t limit);

#endif
