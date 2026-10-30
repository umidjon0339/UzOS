#ifndef SYS_RENAMEAT2_H
#define SYS_RENAMEAT2_H
/* sys_renameat2.h - renameat2(olddirfd, old, newdirfd, new, flags): renameat with options. */
#define SYS_RENAMEAT2 123   // EAX=123, EBX=olddirfd, ECX=old, EDX=newdirfd, ESI=new, EDI=flags
#define RENAME_NOREPLACE 1  // fail if 'new' exists (no silent overwrite)
#define RENAME_EXCHANGE  2  // swap 'old' and 'new', both must exist
int sys_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
#endif
