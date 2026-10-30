#ifndef SYS_EPOLL_CREATE_H
#define SYS_EPOLL_CREATE_H
/* sys_epoll_create.h - epoll_create(size): the old form; size only has to be above 0. */
#include <stdint.h>
#define SYS_EPOLL_CREATE 156    // EAX=156, EBX=size (a hint, unused)
int sys_epoll_create(int size);
#endif
