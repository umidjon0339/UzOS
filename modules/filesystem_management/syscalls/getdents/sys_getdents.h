#ifndef SYS_GETDENTS_H
#define SYS_GETDENTS_H
/* sys_getdents.h - getdents(fd, buf, count): read a directory's entries (32-bit records). */
#include <stdint.h>
#define SYS_GETDENTS 125     // EAX=125, EBX=fd, ECX=buf, EDX=count
#define DT_DIR 4                // d_type
#define DT_REG 8
#define DT_LNK 10
// struct linux_dirent, back to back in 'buf' (each 4-byte aligned):
//   u32 d_ino | u32 d_off | u16 d_reclen | char d_name[] NUL ... | u8 d_type (last byte)
int sys_getdents(int fd, uint8_t *buf, uint32_t count);
#endif
