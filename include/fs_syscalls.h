#ifndef FS_SYSCALLS_H
#define FS_SYSCALLS_H

#include <stdint.h>
#include "io_syscalls.h"

// These syscalls will eventually be called via int 0x80 from user space.
// For now, kernel commands can call them directly.

int sys_mkdir(const char *path);
int sys_unlink(const char *path); // delete file
int sys_chdir(const char *path);
struct vfs_node *sys_getcwd_node(void);
int sys_getcwd_string(char *buf, int max_len);

#endif
