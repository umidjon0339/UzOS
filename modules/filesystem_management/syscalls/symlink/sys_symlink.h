#ifndef SYS_SYMLINK_H
#define SYS_SYMLINK_H
/* sys_symlink.h - symlink(target, linkpath): a name that points to a path. */
#define SYS_SYMLINK 129     // EAX=129, EBX=target, ECX=linkpath
int sys_symlink(const char *target, const char *linkpath);
#endif
