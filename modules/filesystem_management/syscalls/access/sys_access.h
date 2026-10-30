#ifndef SYS_ACCESS_H
#define SYS_ACCESS_H
/* sys_access.h - access(path, mode): may I read / write / run this file? */
#define SYS_ACCESS 102      // EAX=102, EBX=path, ECX=mode
#define F_OK 0              // does it exist?
#define X_OK 1              // may I run it (UzOS: read it) / search it?
#define W_OK 2
#define R_OK 4
int sys_access(const char *path, int mode);
#endif
