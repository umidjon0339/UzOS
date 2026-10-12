#include "vfs.h"

vfs_node_t *fs_root = 0; // The root of the filesystem

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->read != 0) return node->read(node, offset, size, buffer);
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->write != 0) return node->write(node, offset, size, buffer);
    return 0;
}

void vfs_open(vfs_node_t *node) {
    if (node->open != 0) node->open(node);
}

void vfs_close(vfs_node_t *node) {
    if (node->close != 0) node->close(node);
}

vfs_node_t *vfs_readdir(vfs_node_t *node, uint32_t index) {
    if ((node->flags & FS_DIRECTORY) && node->readdir != 0)
        return node->readdir(node, index);
    return 0;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name) {
    if ((node->flags & FS_DIRECTORY) && node->finddir != 0)
        return node->finddir(node, name);
    return 0;
}
