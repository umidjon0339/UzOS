#ifndef RAMDISK_H
#define RAMDISK_H

#include "vfs.h"

void ramdisk_init(void);
vfs_node_t *ramdisk_create_file(vfs_node_t *dir, const char *name);
vfs_node_t *ramdisk_create_dir(vfs_node_t *dir, const char *name);
int ramdisk_delete(vfs_node_t *dir, const char *name);

#endif
