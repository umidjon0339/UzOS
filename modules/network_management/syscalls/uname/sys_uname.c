/*
 * sys_uname.c - uname(buf): fill in the system's names.
 *
 * `uname -a` on Linux prints exactly these six strings. The hostname
 * (nodename) is the one sethostname() set.
 *
 * Uses: hostname_get / domainname_get (core/hostname), strncpy, memset.
 */
#include "modules/network_management/syscalls/uname/sys_uname.h"            // this file
#include "modules/network_management/core/hostname/hostname.h"              // hostname_get, domainname_get
#include "lib/string/string.h"                                              // strncpy, memset

int sys_uname(utsname_t *buf) {
    memset(buf, 0, sizeof(*buf));
    strncpy(buf->sysname, "UzOS", UTS_LEN - 1);
    strncpy(buf->nodename, hostname_get(), UTS_LEN - 1);
    strncpy(buf->release, "1.0", UTS_LEN - 1);
    strncpy(buf->version, "#1 network_management", UTS_LEN - 1);
    strncpy(buf->machine, "i686", UTS_LEN - 1);
    strncpy(buf->domainname, domainname_get(), UTS_LEN - 1);
    return 0;
}
