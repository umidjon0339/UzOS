/*
 * def_descriptor_bounds.c - DEFENCE descriptor_bounds: only real descriptors.
 *
 * Stops:  invalid_descriptor (security/attack/invalid_descriptor)
 * Called: sys_read, sys_write, sys_close, sys_seek (syscalls/), first thing.
 * Rules:  1. 0 <= fd < MAX_FDS (it indexes the fd_table array)
 *         2. stdin can only be read, stdout/stderr only written
 *         3. a file descriptor must be open, and opened for what you do
 *            (no write() on a file opened with O_READ only)
 * Why:    fd is a number from the program. fd = -1 or 9999 indexes
 *         fd_table outside its 16 entries and reads a random pointer as
 *         a "file". A closed descriptor holds NULL.
 */

#include "def_descriptor_bounds.h"
#include "registry.h"
#include "audit.h"
#include "sys_open.h"
#include "fd.h"

// Why the use is refused, or 0 if fine. A pure function of numbers.
static const char *refusal(int fd, int is_open, uint32_t flags, uint32_t need) {
    if (fd < 0 || fd >= MAX_FDS)        return "descriptor number out of range";
    if (fd == STDIN)                    return need == O_READ ? 0 : "stdin can only be read";
    if (fd == STDOUT || fd == STDERR)   return need == O_WRITE ? 0 : "stdout/stderr can only be written";
    if (!is_open)                       return "descriptor is not open";
    if ((flags & need) != need)         return "file was not opened for this";
    return 0;
}

int def_fd_ok(const process_t *p, int fd, uint32_t need) {
    int in_range = fd >= 0 && fd < MAX_FDS;
    const char *reason = !p ? "no current process"
                       : refusal(fd, in_range && p->fd_table[fd] != 0,
                                 in_range ? p->fd_flags[fd] : 0, need);
    if (!reason)
        return 1;
    audit_log("descriptor_bounds", reason);
    return 0;
}

static int selftest(void) {
    if (refusal(-1, 0, 0, O_READ) == 0) return 0;           // negative
    if (refusal(MAX_FDS, 0, 0, O_READ) == 0) return 0;      // past the table
    if (refusal(STDIN, 0, 0, O_READ) != 0) return 0;        // read stdin: ok
    if (refusal(STDIN, 0, 0, O_WRITE) == 0) return 0;       // write stdin
    if (refusal(STDOUT, 0, 0, O_WRITE) != 0) return 0;      // write stdout: ok
    if (refusal(5, 0, 0, O_READ) == 0) return 0;            // not open
    if (refusal(5, 1, O_READ, O_READ) != 0) return 0;       // open for reading: ok
    if (refusal(5, 1, O_READ, O_WRITE) == 0) return 0;      // read-only file
    return 1;
}

static const defence_t defence = {
    "descriptor_bounds",
    "only use descriptors that exist, are open, and allow the operation",
    "invalid_descriptor",
    selftest
};

void def_descriptor_bounds_register(void) {
    registry_add_defence(&defence);
}
