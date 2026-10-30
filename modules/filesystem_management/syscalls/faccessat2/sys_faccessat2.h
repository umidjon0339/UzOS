#ifndef SYS_FACCESSAT2_H
#define SYS_FACCESSAT2_H
/* sys_faccessat2.h - faccessat2(dirfd, path, mode, flags): faccessat with flags. */
#define SYS_FACCESSAT2 104  // EAX=104, EBX=dirfd, ECX=path, EDX=mode, ESI=flags
#define AT_EACCESS          0x200   // check with the effective ids (UzOS: the same)
#define AT_SYMLINK_NOFOLLOW_A 0x100 // check a final symbolic link itself
int sys_faccessat2(int dirfd, const char *path, int mode, int flags);
#endif
