#ifndef MEM_SYSCALLS_H
#define MEM_SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

void *sys_sbrk(int incr);
void *sys_mmap(void *addr, size_t length, int prot, int flags);
int   sys_munmap(void *addr, size_t length);

#endif
