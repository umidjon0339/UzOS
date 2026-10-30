#ifndef SYS_TRUNCATE_H
#define SYS_TRUNCATE_H
/* sys_truncate.h - truncate(path, length): make a file exactly this long. */
#include <stdint.h>
#define SYS_TRUNCATE 113    // EAX=113, EBX=path, ECX=length
int sys_truncate(const char *path, uint32_t length);
#endif
