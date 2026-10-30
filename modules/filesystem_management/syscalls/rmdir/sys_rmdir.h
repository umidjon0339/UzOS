#ifndef SYS_RMDIR_H
#define SYS_RMDIR_H
/* sys_rmdir.h - rmdir(path): remove an empty directory. */
#define SYS_RMDIR 119       // EAX=119, EBX=path
int sys_rmdir(const char *path);
#endif
