#ifndef SYS_UNSHARE_H
#define SYS_UNSHARE_H
/* sys_unshare.h - unshare(flags): stop sharing something with other processes. */
#include <stdint.h>
#define SYS_UNSHARE 91          // EAX=91, EBX=flags (CLONE_*)

#define UNSHARE_FS       0x00000200u    // CLONE_FS: current directory
#define UNSHARE_FILES    0x00000400u    // CLONE_FILES: the descriptor table
#define UNSHARE_SYSVSEM  0x00040000u
#define UNSHARE_NEWNS    0x00020000u    // the namespaces UzOS does not have
#define UNSHARE_NEWCGROUP 0x02000000u
#define UNSHARE_NEWUTS   0x04000000u
#define UNSHARE_NEWIPC   0x08000000u
#define UNSHARE_NEWUSER  0x10000000u
#define UNSHARE_NEWPID   0x20000000u
#define UNSHARE_NEWNET   0x40000000u

int sys_unshare(uint32_t flags);
#endif
