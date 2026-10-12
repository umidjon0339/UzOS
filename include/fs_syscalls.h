#ifndef FS_SYSCALLS_H
#define FS_SYSCALLS_H

#include <stdint.h>

// These syscalls will eventually be called via int 0x80 from user space.
// For now, kernel commands can call them directly.

int sys_open(const char *path);
int sys_read(int fd, void *buf, uint32_t count);
int sys_write(int fd, const void *buf, uint32_t count);
int sys_close(int fd);
int sys_mkdir(const char *path);
int sys_unlink(const char *path); // delete file
int sys_chdir(const char *path);
struct vfs_node *sys_getcwd_node(void);
int sys_getcwd_string(char *buf, int max_len);

#endif
