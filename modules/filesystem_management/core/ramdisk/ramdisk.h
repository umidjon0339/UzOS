#ifndef RAMDISK_H
#define RAMDISK_H

/*
 * ramdisk.h - A filesystem that lives entirely in kernel memory.
 *
 * Every file's bytes are a kmalloc() buffer that grows as you write.
 * Nothing survives a reboot. It is the "driver" behind the VFS: it fills
 * in each node's read/write/readdir/finddir functions.
 */

#include "modules/filesystem_management/core/vfs/vfs.h"  // vfs_node_t

#define RAMDISK_MAX_FILE_SIZE (64 * 1024)   // one file can grow to 64 KiB

// Make the root "/" and put the demo program /hello in it.
void ramdisk_init(void);

// New empty file or directory 'name' in 'dir' (type FS_FILE / FS_DIRECTORY).
// NULL if the name is bad (filename_check), taken, or the directory is full.
vfs_node_t *ramdisk_create(vfs_node_t *dir, const char *name, uint32_t type);

// Remove 'name' from 'dir'. Refused (-1) while it is in use (busy_check).
// A file's bytes stay while another name or an open descriptor has it.
int ramdisk_delete(vfs_node_t *dir, const char *name);

// A new name 'name' in 'dir' for the file 'inode' (hard link). NULL if
// refused (bad / taken name, too many names: nlink_bounds). The caller
// asks protected_hardlinks first.
vfs_node_t *ramdisk_link(vfs_node_t *dir, const char *name, vfs_node_t *inode);

// A symbolic link 'name' in 'dir' that points at 'target' (any text: it
// is followed when a path walks through it). NULL if refused.
vfs_node_t *ramdisk_symlink(vfs_node_t *dir, const char *name, const char *target);

// Move/rename 'node' into 'new_dir' as 'new_name'.
// Refused (-1) for a bad name, a taken name, or a loop (no_cycle).
int ramdisk_move(vfs_node_t *node, vfs_node_t *new_dir, const char *new_name);

// Swap two entries in one step (renameat2 RENAME_EXCHANGE): each takes
// the other's place and name. Refused (-1) if one is inside the other, a
// result breaks path_length / depth_limit, or one is not a RAM disk node.
int ramdisk_exchange(vfs_node_t *a, vfs_node_t *b);

// Empty a file (size 0); used when a file is opened for rewriting.
void ramdisk_truncate(vfs_node_t *file);

// Make a file exactly 'size' bytes long: cut it, or extend it with zero
// bytes (truncate, ftruncate, fallocate). 0, or -1 (not a RAM disk file,
// too big, out of memory). The caller asks size_cap first.
int ramdisk_resize(vfs_node_t *file, uint32_t size);

#endif
