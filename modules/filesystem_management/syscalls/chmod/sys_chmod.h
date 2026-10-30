#ifndef SYS_CHMOD_H
#define SYS_CHMOD_H
/* sys_chmod.h - chmod(path, mode): change who may read and write a file. */
#include <stdint.h>
#define SYS_CHMOD 105       // EAX=105, EBX=path, ECX=mode (Linux bits: 0644)
int sys_chmod(const char *path, uint32_t mode);
#endif
