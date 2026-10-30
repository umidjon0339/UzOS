/*
 * sys_gethostname.c - gethostname(buf, len): copy the hostname out.
 *
 * (On Linux this is a C library function made from uname(); UzOS has
 * it as a syscall so a small program can ask directly.)
 *
 * Uses: hostname_get (core/hostname), strlen / memcpy (lib/string).
 */
#include "modules/network_management/syscalls/gethostname/sys_gethostname.h"  // this file
#include "modules/network_management/core/hostname/hostname.h"              // hostname_get
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "lib/string/string.h"                                              // strlen, memcpy

int sys_gethostname(char *buf, uint32_t len) {
    const char *name = hostname_get();
    uint32_t n = strlen(name) + 1;              // with its NUL
    if (len < n)
        return -NET_EINVAL;
    memcpy(buf, name, n);
    return 0;
}
