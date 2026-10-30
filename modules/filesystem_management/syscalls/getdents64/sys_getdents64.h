#ifndef SYS_GETDENTS64_H
#define SYS_GETDENTS64_H
/* sys_getdents64.h - getdents64(fd, buf, count): read a directory's entries (64-bit records). */
#include <stdint.h>
#define SYS_GETDENTS64 126     // EAX=126, EBX=fd, ECX=buf, EDX=count
#define DT_DIR 4                // d_type
#define DT_REG 8
#define DT_LNK 10
// struct linux_dirent64, back to back in 'buf' (each 8-byte aligned):
//   u64 d_ino | s64 d_off | u16 d_reclen | u8 d_type | char d_name[] NUL
int sys_getdents64(int fd, uint8_t *buf, uint32_t count);
#endif
