#ifndef PERM_H
#define PERM_H

/*
 * perm.h - Who may read or change a file.
 *
 * Every file and directory has an OWNER (a uid) and a MODE: two octal
 * digits, one for the owner and one for everybody else.
 *
 *     digit = 4 (read) + 2 (write)          mode 64 = owner rw, others r
 *                                           shown as  rw/r-
 *     6 = rw    4 = r-    2 = -w    0 = --
 *
 * What read and write mean:
 *
 *                 read                     write
 *     file        cat, cp from, exec       write, cp to, change it
 *     directory   ls, cd into it           create or delete things in it
 *
 * Deleting or renaming also needs you to OWN the thing (or be root):
 * write rights on a shared directory are not enough to remove someone
 * else's file.
 *
 * root (uid 0) passes every check. (Linux has a third digit, for a
 * "group", and an x = execute bit; UzOS keeps just these two.)
 *
 * The filesystem syscalls call these checks (filesystem_management,
 * io_management); the rule itself is the defence file_access.
 */

#include <stdint.h>
#include "vfs.h"

#define PERM_READ     4
#define PERM_WRITE    2

#define MODE_DEFAULT  064           // owner rw, others r: new files and directories
#define MODE_PRIVATE  060           // owner rw, others nothing

// May the calling process do 'want' (PERM_READ / PERM_WRITE) to 'node'?
// 1 = yes, 0 = refused (logged by file_access).
int perm_check(const vfs_node_t *node, int want);

// May the caller remove 'node' from 'dir' (delete, or move it away)?
// Needs write on 'dir' and owning 'node' (or root).
int perm_may_remove(const vfs_node_t *dir, const vfs_node_t *node);

// May the caller change 'node's mode? Its owner and root may.
int perm_may_chmod(const vfs_node_t *node);

// "rw/r-" for mode 064.
void perm_format(uint32_t mode, char out[6]);

// Read a mode like "64" or "60". 1 = valid (written to *mode).
int perm_parse(const char *text, uint32_t *mode);

#endif
