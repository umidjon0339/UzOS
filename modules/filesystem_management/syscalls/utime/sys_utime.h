#ifndef SYS_UTIME_H
#define SYS_UTIME_H
/* sys_utime.h - utime(path, times): set a file's access and change times. */
#include <stdint.h>
#define SYS_UTIME 116       // EAX=116, EBX=path, ECX=times (or 0 = now)
typedef struct {
    uint32_t actime;        // last access
    uint32_t modtime;       // last modification
} utimbuf_t;
int sys_utime(const char *path, const utimbuf_t *times);
#endif
