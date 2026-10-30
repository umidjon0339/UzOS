#ifndef SYS_UTIMENSAT_H
#define SYS_UTIMENSAT_H
/* sys_utimensat.h - utimensat(dirfd, path, ts[2], flags): set times, nanoseconds, each optional. */
#include <stdint.h>
#define SYS_UTIMENSAT 118   // EAX=118, EBX=dirfd, ECX=path, EDX=ts (or 0), ESI=flags
#define UTIME_NOW  0x3FFFFFFF       // in ts[i].nsec: this one becomes now
#define UTIME_OMIT 0x3FFFFFFE       // in ts[i].nsec: leave this one alone
typedef struct {
    uint32_t sec;
    uint32_t nsec;
} utimensat_ts_t;
int sys_utimensat(int dirfd, const char *path, const utimensat_ts_t ts[2], int flags);
#endif
