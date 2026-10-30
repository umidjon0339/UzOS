#ifndef SYS_LINKAT_H
#define SYS_LINKAT_H
/* sys_linkat.h - linkat(olddirfd, old, newdirfd, new, flags): link() relative to directories. */
#define SYS_LINKAT 128      // EAX=128, EBX=olddirfd, ECX=old, EDX=newdirfd, ESI=new, EDI=flags
#define AT_SYMLINK_FOLLOW 0x400     // follow a final symbolic link in 'old'
int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
#endif
