#ifndef SYS_RENAMEAT_H
#define SYS_RENAMEAT_H
/* sys_renameat.h - renameat(olddirfd, old, newdirfd, new): rename relative to directories. */
#define SYS_RENAMEAT 122    // EAX=122, EBX=olddirfd, ECX=old, EDX=newdirfd, ESI=new
int sys_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
#endif
