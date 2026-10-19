#ifndef IO_SYSCALLS_H
#define IO_SYSCALLS_H

#include <stdint.h>

int sys_open(const char *path);
int sys_read(int fd, void *buf, uint32_t count);
int sys_write(int fd, const void *buf, uint32_t count);
int sys_close(int fd);

#endif
