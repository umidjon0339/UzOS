#ifndef SYS_LINK_H
#define SYS_LINK_H
/* sys_link.h - link(old, new): give a file a second name (a hard link). */
#define SYS_LINK 127        // EAX=127, EBX=old, ECX=new
int sys_link(const char *oldpath, const char *newpath);
#endif
