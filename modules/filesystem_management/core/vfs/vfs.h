#ifndef VFS_H
#define VFS_H

/*
 * vfs.h - Virtual File System: files and directories as a tree of nodes.
 *
 *            /                 every node has a name, a parent, and
 *          /   \               (for directories) children.
 *       hello   docs           Paths walk this tree:
 *              /    \            "/docs/a.txt"   from the root
 *           a.txt   b.txt        "../hello"      from the current directory
 *
 * The VFS only knows nodes and paths. Storing the bytes is the job of a
 * filesystem DRIVER, which plugs in through the read/write/readdir/finddir
 * function pointers. UzOS has one driver: the RAM disk (core/ramdisk).
 */

#include <stdint.h>

#define FS_FILE         0x01
#define FS_DIRECTORY    0x02

#define FS_NAME_MAX     127     // longest file name (one path component)
#define FS_PATH_MAX     256     // longest full path
#define FS_MAX_CHILDREN 64      // entries per directory

struct vfs_node;
typedef uint32_t (*vfs_read_t)(struct vfs_node *, uint32_t offset, uint32_t size, uint8_t *buf);
typedef uint32_t (*vfs_write_t)(struct vfs_node *, uint32_t offset, uint32_t size, uint8_t *buf);
typedef struct vfs_node *(*vfs_readdir_t)(struct vfs_node *, uint32_t index);
typedef struct vfs_node *(*vfs_finddir_t)(struct vfs_node *, const char *name);

typedef struct vfs_node {
    char     name[FS_NAME_MAX + 1];
    uint32_t flags;             // FS_FILE or FS_DIRECTORY
    uint32_t size;              // bytes of data (files)
    uint32_t open_count;        // descriptors pointing here (see busy_check)

    // Who may use it, checked by user_management/core/perm.
    uint32_t owner;             // uid of the user it belongs to
    uint32_t mode;              // e.g. 064: owner may read+write, others read

    // Filled in by the driver.
    vfs_read_t    read;
    vfs_write_t   write;
    vfs_readdir_t readdir;
    vfs_finddir_t finddir;
    uint8_t  *data;             // driver's storage for the file's bytes
    uint32_t  capacity;         // bytes allocated at 'data'

    struct vfs_node *parent;    // the root is its own parent
    struct vfs_node *children[FS_MAX_CHILDREN];
    int num_children;
} vfs_node_t;

extern vfs_node_t *fs_root;

/* ---------- Using a node ---------- */

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf);
vfs_node_t *vfs_readdir(vfs_node_t *dir, uint32_t index);   // NULL past the end
vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name); // one name, no '/'

// A descriptor starts / stops pointing at 'node'.
void vfs_open(vfs_node_t *node);
void vfs_close(vfs_node_t *node);

/* ---------- Paths ---------- */

// The calling process's current directory (the root if it has none).
vfs_node_t *vfs_cwd(void);

// Follow 'path' from 'start' (or from the root if it begins with '/').
// Handles "." and "..". Returns NULL if anything on the way is missing.
vfs_node_t *vfs_lookup(vfs_node_t *start, const char *path);

// For creating, deleting and renaming: find the directory that will
// hold the LAST name of 'path', and copy that name to 'name_out'
// (FS_NAME_MAX + 1 bytes). Returns NULL if that directory is missing.
vfs_node_t *vfs_lookup_parent(vfs_node_t *start, const char *path, char *name_out);

// Write the absolute path of 'node' ("/docs/a.txt"). 0 on success, -1 if too long.
int vfs_path_of(vfs_node_t *node, char *buf, int max);

// Is 'ancestor' the same node as 'node', or one of its parents?
int vfs_is_ancestor(vfs_node_t *ancestor, vfs_node_t *node);

#endif
