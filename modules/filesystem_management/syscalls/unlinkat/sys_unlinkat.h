#ifndef SYS_UNLINKAT_H
#define SYS_UNLINKAT_H
/* sys_unlinkat.h - unlinkat(dirfd, path, flags): unlink or rmdir relative to a directory. */
#define SYS_UNLINKAT 121    // EAX=121, EBX=dirfd, ECX=path, EDX=flags
#define AT_REMOVEDIR 0x200  // act as rmdir()
int sys_unlinkat(int dirfd, const char *path, int flags);
#endif
