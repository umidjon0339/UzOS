#ifndef SYS_CLOSE_H
#define SYS_CLOSE_H

/*
 * sys_close.h - close(): give a descriptor back.
 */

#define SYS_CLOSE 17    // int 0x80 with EAX = 17, EBX = fd

// 0, or -1 for a descriptor that is not an open file.
int sys_close(int fd);

#endif
