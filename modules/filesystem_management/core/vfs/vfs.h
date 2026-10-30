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
#define FS_LINK         0x04    // a symbolic link (phase "links")

#define VFS_DEV_RAMDISK 1       // vfs_node_t.dev: the RAM disk (/)
#define VFS_DEV_PROC    2       //                 /proc

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

    // The inode part: what `stat` reports.
    uint32_t dev;               // which driver made it: VFS_DEV_RAMDISK, VFS_DEV_PROC
    uint32_t ino;               // its number, never reused (st_ino)
    uint32_t nlink;             // names that point at it (1: links come later)
    uint32_t atime;             // last read            (seconds since 1970)
    uint32_t mtime;             // last content change
    uint32_t ctime;             // last change of anything: content, mode, owner, name

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

    struct vfs_node *target;    // a hard link: this ENTRY is another name of 'target' (the inode)
    void (*release)(struct vfs_node *);     // the driver frees an inode no one refers to

    struct vfs_node *parent;    // the root is its own parent; NULL = no name left
    struct vfs_node *children[FS_MAX_CHILDREN];
    int num_children;
} vfs_node_t;

extern vfs_node_t *fs_root;

/* ---------- Entries and inodes ---------- */

// A directory ENTRY is a name; the INODE is the file it names. Usually
// they are one node. A hard link is an extra entry node whose 'target'
// is the inode: everything about the file (bytes, size, owner, times)
// lives in the inode.
static inline vfs_node_t *vfs_inode(vfs_node_t *entry) {
    return entry && entry->target ? entry->target : entry;
}

/* ---------- Using a node ---------- */

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf);
vfs_node_t *vfs_readdir(vfs_node_t *dir, uint32_t index);   // NULL past the end
vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name); // one name, no '/'

// A descriptor starts / stops pointing at 'node'.
void vfs_open(vfs_node_t *node);
void vfs_close(vfs_node_t *node);

/* ---------- Times and stat ---------- */

// The time now, in seconds since 1970: the RTC read once, then counted
// on with the timer (reading the RTC on every file access would be slow).
uint32_t vfs_now(void);

// Linux's struct stat, as UzOS fills it (all 32-bit fields).
typedef struct {
    uint32_t dev;               // which filesystem: VFS_DEV_RAMDISK, VFS_DEV_PROC
    uint32_t ino;
    uint32_t mode;              // S_IF* type | rwx bits (see vfs_stat)
    uint32_t nlink;
    uint32_t uid, gid;          // gid = uid: UzOS has no groups
    uint32_t rdev;
    uint32_t size;
    uint32_t blksize, blocks;   // 4096; size in 512-byte blocks
    uint32_t atime, mtime, ctime;
} vfs_stat_t;

#define S_IFMT   0170000
#define S_IFDIR  0040000
#define S_IFREG  0100000
#define S_IFLNK  0120000

// A new inode number for a new node: 1, 2, 3, ... never reused.
uint32_t vfs_new_ino(void);

// Fill 'st' from 'node'. UzOS's mode has two digits (owner, others):
// owner rw -> 0600, others rw -> group AND others 0066; a directory you
// may read may also be searched (x).
void vfs_stat(const vfs_node_t *node, vfs_stat_t *st);

/* ---------- Paths ---------- */

#define AT_FDCWD (-100)     // "*at" syscalls: relative to the current directory

// Where an "*at" syscall starts a relative path: the current directory
// for AT_FDCWD, else the directory open as descriptor 'dirfd'. NULL if
// 'dirfd' is not an open directory.
vfs_node_t *vfs_start_dir(int dirfd);

// The calling process's current directory (the root if it has none).
vfs_node_t *vfs_cwd(void);

// Follow 'path' from 'start' (or from the root if it begins with '/').
// Handles "." and "..", and follows symbolic links (link_depth,
// protected_symlinks). Returns the INODE it reaches (a hard link's
// target), or NULL if anything on the way is missing or refused.
vfs_node_t *vfs_lookup(vfs_node_t *start, const char *path);

// The same, but a symbolic link as the LAST name is returned itself, not
// followed (lstat, readlink, lchown, unlink of a link).
vfs_node_t *vfs_lookup_nofollow(vfs_node_t *start, const char *path);

// The ENTRY 'path' names: the directory entry itself - a hard link's own
// name node, a symbolic link not followed. rename and unlink act on
// entries. ("/", "." and ".." name no entry: their node is returned.)
vfs_node_t *vfs_lookup_entry(vfs_node_t *start, const char *path);

// For creating, deleting and renaming: find the directory that will
// hold the LAST name of 'path', and copy that name to 'name_out'
// (FS_NAME_MAX + 1 bytes). Returns NULL if that directory is missing.
vfs_node_t *vfs_lookup_parent(vfs_node_t *start, const char *path, char *name_out);

// Write the absolute path of 'node' ("/docs/a.txt"). 0 on success, -1 if too long.
int vfs_path_of(vfs_node_t *node, char *buf, int max);

// Is 'ancestor' the same node as 'node', or one of its parents?
int vfs_is_ancestor(vfs_node_t *ancestor, vfs_node_t *node);

#endif
