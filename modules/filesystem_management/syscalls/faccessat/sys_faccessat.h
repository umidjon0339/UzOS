#ifndef SYS_FACCESSAT_H
#define SYS_FACCESSAT_H
/* sys_faccessat.h - faccessat(dirfd, path, mode): access() relative to a directory. */
#define SYS_FACCESSAT 103   // EAX=103, EBX=dirfd, ECX=path, EDX=mode
int sys_faccessat(int dirfd, const char *path, int mode);
#endif
