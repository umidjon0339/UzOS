#ifndef RAMDISK_H
#define RAMDISK_H

/*
 * ramdisk.h - A filesystem that lives entirely in kernel memory.
 *
 * Every file's bytes are a kmalloc() buffer that grows as you write.
 * Nothing survives a reboot. It is the "driver" behind the VFS: it fills
 * in each node's read/write/readdir/finddir functions.
 */

#include "vfs.h"

#define RAMDISK_MAX_FILE_SIZE (64 * 1024)   // one file can grow to 64 KiB

// Make the root "/" and put the demo program /hello in it.
void ramdisk_init(void);

// New empty file or directory 'name' in 'dir' (type FS_FILE / FS_DIRECTORY).
// NULL if the name is bad (filename_check), taken, or the directory is full.
vfs_node_t *ramdisk_create(vfs_node_t *dir, const char *name, uint32_t type);

// Remove 'name' from 'dir'. Refused (-1) while it is in use (busy_check).
int ramdisk_delete(vfs_node_t *dir, const char *name);

// Move/rename 'node' into 'new_dir' as 'new_name'.
// Refused (-1) for a bad name, a taken name, or a loop (no_cycle).
int ramdisk_move(vfs_node_t *node, vfs_node_t *new_dir, const char *new_name);

// Empty a file (size 0); used when a file is opened for rewriting.
void ramdisk_truncate(vfs_node_t *file);

#endif
