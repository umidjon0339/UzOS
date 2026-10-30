#ifndef SYS_SYMLINKAT_H
#define SYS_SYMLINKAT_H
/* sys_symlinkat.h - symlinkat(target, newdirfd, linkpath): symlink relative to a directory. */
#define SYS_SYMLINKAT 130   // EAX=130, EBX=target, ECX=newdirfd, EDX=linkpath
int sys_symlinkat(const char *target, int newdirfd, const char *linkpath);
#endif
