#ifndef DEF_ARG_LIMITS_H
#define DEF_ARG_LIMITS_H

#include <stdint.h>

#define ARG_MAX_STRINGS 32      // argv + envp entries together
#define ARG_MAX_BYTES   2048    // their bytes together, NULs included (Linux: ARG_MAX)

// Is the NULL-terminated array 'list' (argv or envp) acceptable? From
// ring 3 ('from_user'), every slot and every string must be the caller's
// own mapped memory; in total (with what came before: *count strings,
// *bytes bytes) at most ARG_MAX_STRINGS and ARG_MAX_BYTES. On success
// *count and *bytes are updated. execve asks this for argv, then envp.
// 1 = fine, 0 = refused (logged): E2BIG or EFAULT.
int def_args_ok(char *const list[], int from_user, uint32_t *count, uint32_t *bytes);

#endif
