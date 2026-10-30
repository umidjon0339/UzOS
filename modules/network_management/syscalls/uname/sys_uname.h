#ifndef SYS_UNAME_H
#define SYS_UNAME_H
/* sys_uname.h - uname(buf): what this system is and what it is called. */
#include <stdint.h>
#define SYS_UNAME 161   // EAX=161, EBX=buf (utsname_t)
#define UTS_LEN   65    // each field: 64 characters + NUL, as Linux

// Linux's struct new_utsname: six fixed-size strings.
typedef struct {
    char sysname[UTS_LEN];      // "UzOS"
    char nodename[UTS_LEN];     // the hostname: "uzos"
    char release[UTS_LEN];      // "1.0"
    char version[UTS_LEN];      // "#1 network_management"
    char machine[UTS_LEN];      // "i686"
    char domainname[UTS_LEN];   // "(none)"
} utsname_t;

// 0 (buf filled).
int sys_uname(utsname_t *buf);
#endif
