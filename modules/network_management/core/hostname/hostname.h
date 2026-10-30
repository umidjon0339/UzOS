#ifndef HOSTNAME_H
#define HOSTNAME_H

/*
 * hostname.h - This machine's name ("uzos"), and its domain.
 *
 * The prompt (root@uzos), the login line and uzfetch show it; other
 * machines know us by it. Linux keeps it in the kernel too (the "UTS"
 * part of uname); sethostname() changes it (needs CAP_NET_ADMIN, as
 * Linux's CAP_SYS_ADMIN).
 */

#define HOSTNAME_MAX 64             // 63 characters + NUL, as Linux

// The name, never NULL.
const char *hostname_get(void);
const char *domainname_get(void);   // "(none)" until set

// Change it. 0, or -NET_EINVAL for a bad name (only letters, digits,
// '-' and '.'; 1-63 characters).
int hostname_set(const char *name);
int domainname_set(const char *name);

#endif
