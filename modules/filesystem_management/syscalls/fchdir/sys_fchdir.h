#ifndef SYS_FCHDIR_H
#define SYS_FCHDIR_H
/* sys_fchdir.h - fchdir(fd): go into the directory open as fd. */
#define SYS_FCHDIR 124      // EAX=124, EBX=fd
int sys_fchdir(int fd);
#endif
