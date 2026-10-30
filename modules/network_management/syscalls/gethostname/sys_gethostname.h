#ifndef SYS_GETHOSTNAME_H
#define SYS_GETHOSTNAME_H
/* sys_gethostname.h - gethostname(buf, len): this machine's name. */
#include <stdint.h>
#define SYS_GETHOSTNAME 164   // EAX=164, EBX=buf, ECX=len
// 0, or -EINVAL if buf is too small for the name and its NUL.
int sys_gethostname(char *buf, uint32_t len);
#endif
