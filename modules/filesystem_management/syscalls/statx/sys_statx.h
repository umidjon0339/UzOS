#ifndef SYS_STATX_H
#define SYS_STATX_H
/* sys_statx.h - statx(dirfd, path, flags, mask, buf): stat, the extensible way. */
#include <stdint.h>
#define SYS_STATX 101       // EAX=101, EBX=dirfd, ECX=path, EDX=flags, ESI=mask, EDI=buf

// What statx says it filled in (stx_mask), as Linux.
#define STATX_TYPE   0x001
#define STATX_MODE   0x002
#define STATX_NLINK  0x004
#define STATX_UID    0x008
#define STATX_GID    0x010
#define STATX_ATIME  0x020
#define STATX_MTIME  0x040
#define STATX_CTIME  0x080
#define STATX_INO    0x100
#define STATX_SIZE   0x200
#define STATX_BLOCKS 0x400
#define STATX_BTIME  0x800          // birth time: UzOS keeps none (never in mask)
#define STATX_BASIC_STATS 0x7FF

// struct statx, the fields UzOS fills (times: seconds + nanoseconds).
typedef struct {
    uint32_t mask;
    uint32_t blksize;
    uint32_t nlink, uid, gid;
    uint32_t mode;
    uint32_t ino, size, blocks;
    uint32_t atime_sec, atime_nsec;
    uint32_t btime_sec, btime_nsec;
    uint32_t ctime_sec, ctime_nsec;
    uint32_t mtime_sec, mtime_nsec;
    uint32_t dev;
} statx_t;

int sys_statx(int dirfd, const char *path, int flags, uint32_t mask, statx_t *buf);
#endif
