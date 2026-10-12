#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02

struct vfs_node;

// Function prototypes for filesystem drivers
typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);
typedef struct vfs_node * (*readdir_type_t)(struct vfs_node*, uint32_t);
typedef struct vfs_node * (*finddir_type_t)(struct vfs_node*, const char *name);

typedef struct vfs_node {
    char name[128];     // Node name
    uint32_t flags;     // FS_FILE or FS_DIRECTORY
    uint32_t size;      // Size of the file (in bytes)
    
    // Implementation callbacks provided by the driver (like Ramdisk)
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    
    // Driver-specific pointer (e.g., points to the dynamically allocated file data)
    struct vfs_node *ptr; 
    
    // For simple directory management
    struct vfs_node *parent;
    struct vfs_node *children[64];
    int num_children;
} vfs_node_t;

extern vfs_node_t *fs_root;

// Standard VFS API wrappers
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void vfs_open(vfs_node_t *node);
void vfs_close(vfs_node_t *node);
vfs_node_t *vfs_readdir(vfs_node_t *node, uint32_t index);
vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name);

#endif
