/*
 * ramdisk.c - Store files and directories in the kernel heap.
 *
 * Every new node belongs to the user who created it (process_caller_uid)
 * and starts with the default mode (user_management/core/perm).
 *
 * Every node is stamped: its own inode number, atime on read, mtime and
 * ctime on write / resize, ctime on a rename, and a directory's times
 * when its entries change (vfs_now, core/vfs).
 *
 * Uses: kmalloc/kfree (memory_management), memcpy/strcmp (lib/string),
 *       vfs_now / vfs_new_ino (core/vfs), the creator's umask
 *       (process_management), and nine defences:
 *         filename_check - names for new or renamed entries
 *         file_range     - offset + size never overflows or passes the limit
 *         busy_check     - never delete what someone is still using
 *         no_cycle       - never move a directory inside itself
 *         secure_umask   - a new node is never writable by others
 *         path_length    - every node's full path fits 255 bytes
 *         depth_limit    - directories nest 32 deep at most
 *         nlink_bounds   - a file has at most 16 names
 *         inode_refcount - a file's bytes go only at 0 names and 0 fds
 *
 * Links: a hard link is an extra ENTRY node whose 'target' is the file
 * (the inode); a symbolic link is an FS_LINK node holding its target
 * path as its bytes. Deleting a file's own name while other names exist
 * leaves the inode alive, nameless (parent NULL).
 */

#include "modules/filesystem_management/core/ramdisk/ramdisk.h"                                             // this file
#include "modules/memory_management/core/kheap/kheap.h"                                                     // kmalloc, kfree
#include "lib/string/string.h"                                                                              // memcpy, memset, strcmp
#include "modules/filesystem_management/core/ramdisk/hello_elf.h"                                           // userland_hello_elf
#include "modules/process_management/core/process/process.h"                                                // process_caller_uid
#include "modules/filesystem_management/security/nlink_overflow/defence/nlink_bounds/def_nlink_bounds.h"  // def_nlink_ok
#include "modules/filesystem_management/security/use_after_unlink/defence/inode_refcount/def_inode_refcount.h"  // def_inode_free_ok
#include "modules/filesystem_management/security/long_path/defence/path_length/def_path_length.h"           // def_path_fits
#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"        // def_depth_ok
#include "modules/filesystem_management/security/umask_weak/defence/secure_umask/def_secure_umask.h"        // def_creation_mode
#include "modules/process_management/core/scheduler/scheduler.h"                                            // scheduler_get_current_process
#include "modules/filesystem_management/security/bad_filename/defence/filename_check/def_filename_check.h"  // def_new_name_ok
#include "modules/filesystem_management/security/file_offset_wrap/defence/file_range/def_file_range.h"      // def_file_range_ok
#include "modules/filesystem_management/security/delete_open_file/defence/busy_check/def_busy_check.h"      // def_delete_allowed
#include "modules/filesystem_management/security/dir_cycle/defence/no_cycle/def_no_cycle.h"                 // def_move_allowed

#define FIRST_CAPACITY 256          // a new file starts with 256 bytes of room

/* ---------- Driver functions (called through the VFS) ---------- */

static uint32_t ramdisk_read(vfs_node_t *file, uint32_t offset, uint32_t size, uint8_t *buf) {
    if (!(file->flags & FS_FILE) || !buf || offset >= file->size)
        return 0;
    if (size > file->size - offset)
        size = file->size - offset;     // stop at the end of the file

    memcpy(buf, file->data + offset, size);
    file->atime = vfs_now();
    return size;
}

// Make room for at least 'needed' bytes, doubling the buffer each time.
static int grow(vfs_node_t *file, uint32_t needed) {
    if (needed <= file->capacity)
        return 1;

    uint32_t capacity = file->capacity ? file->capacity : FIRST_CAPACITY;
    while (capacity < needed)
        capacity *= 2;
    if (capacity > RAMDISK_MAX_FILE_SIZE)
        capacity = RAMDISK_MAX_FILE_SIZE;

    uint8_t *bigger = kmalloc(capacity);
    if (!bigger)
        return 0;
    memset(bigger, 0, capacity);
    if (file->data) {
        memcpy(bigger, file->data, file->size);
        kfree(file->data);
    }
    file->data = bigger;
    file->capacity = capacity;
    return 1;
}

static uint32_t ramdisk_write(vfs_node_t *file, uint32_t offset, uint32_t size, uint8_t *buf) {
    if (!(file->flags & FS_FILE) || !buf || size == 0)
        return 0;

    // DEFENCE file_range: offset + size must fit in a file, without wrapping.
    if (!def_file_range_ok(offset, size, RAMDISK_MAX_FILE_SIZE))
        return 0;
    if (!grow(file, offset + size))
        return 0;

    memcpy(file->data + offset, buf, size);
    if (offset + size > file->size)
        file->size = offset + size;
    file->mtime = file->ctime = vfs_now();
    return size;
}

static vfs_node_t *ramdisk_readdir(vfs_node_t *dir, uint32_t index) {
    return index < (uint32_t)dir->num_children ? dir->children[index] : 0;
}

static vfs_node_t *ramdisk_finddir(vfs_node_t *dir, const char *name) {
    for (int i = 0; i < dir->num_children; i++)
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    return 0;
}

/* ---------- Directory entries ---------- */

static int add_child(vfs_node_t *dir, vfs_node_t *child) {
    if (dir->num_children >= FS_MAX_CHILDREN)
        return 0;
    dir->children[dir->num_children++] = child;
    child->parent = dir;
    return 1;
}

static void remove_child(vfs_node_t *dir, vfs_node_t *child) {
    for (int i = 0; i < dir->num_children; i++) {
        if (dir->children[i] != child)
            continue;
        for (int j = i; j < dir->num_children - 1; j++)
            dir->children[j] = dir->children[j + 1];   // close the gap
        dir->num_children--;
        return;
    }
}

// Free a file's bytes and its node: inode_refcount said nothing refers to it.
static void ramdisk_release(vfs_node_t *node) {
    kfree(node->data);
    kfree(node);
}

// A directory's entries changed (create, delete, move): its times move on.
static void dir_changed(vfs_node_t *dir) {
    dir->mtime = dir->ctime = vfs_now();
}

static vfs_node_t *new_node(const char *name, uint32_t type) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node)
        return 0;
    memset(node, 0, sizeof(*node));
    strcpy(node->name, name);       // length already checked by filename_check
    node->flags = type;
    node->dev   = VFS_DEV_RAMDISK;
    node->ino   = vfs_new_ino();
    node->nlink = 1;
    node->atime = node->mtime = node->ctime = vfs_now();
    node->owner = process_caller_uid();     // whoever creates it owns it

    // DEFENCE secure_umask: the creator's umask, and never others-write.
    process_t *self = scheduler_get_current_process();
    node->mode = def_creation_mode(MODE_CREATE_BASE, self ? self->umask : 002);

    if (type == FS_FILE) {
        node->read    = ramdisk_read;
        node->write   = ramdisk_write;
        node->release = ramdisk_release;    // freed when no name and no fd is left
    } else if (type == FS_DIRECTORY) {
        node->readdir = ramdisk_readdir;
        node->finddir = ramdisk_finddir;
    }                                       // FS_LINK: only its target text in 'data'
    return node;
}

// How many levels a directory reaches below itself (a file: 0).
static uint32_t height_below(const vfs_node_t *node) {
    uint32_t h = 0;
    if (node->flags & FS_DIRECTORY)
        for (int i = 0; i < node->num_children; i++) {
            uint32_t c = 1 + height_below(node->children[i]);   // depth_limit bounds this
            if (c > h)
                h = c;
        }
    return h;
}

// The longest "/a/b/c" below a directory, in bytes (a file: 0).
static uint32_t longest_below(const vfs_node_t *node) {
    uint32_t longest = 0;
    if (node->flags & FS_DIRECTORY)
        for (int i = 0; i < node->num_children; i++) {
            uint32_t c = 1 + strlen(node->children[i]->name) + longest_below(node->children[i]);
            if (c > longest)
                longest = c;
        }
    return longest;
}

/* ---------- Public ---------- */

// Is 'dir' one of OUR directories? Other drivers (cpu_management's
// /proc) hang their nodes into the tree too; the RAM disk must never
// add to, delete from or move their nodes: it did not make them.
static int ours(const vfs_node_t *dir) {
    return dir && dir->finddir == ramdisk_finddir;
}

vfs_node_t *ramdisk_create(vfs_node_t *dir, const char *name, uint32_t type) {
    if (!ours(dir) || !(dir->flags & FS_DIRECTORY))
        return 0;

    // DEFENCE filename_check: no "", ".", "..", "/" or over-long names.
    if (!def_new_name_ok(name))
        return 0;
    if (ramdisk_finddir(dir, name))
        return 0;                   // already exists
    // DEFENCE path_length + depth_limit: every path printable, every tree walkable.
    if (!def_path_fits(dir, name, 0) || !def_depth_ok(dir, 1))
        return 0;

    vfs_node_t *node = new_node(name, type);
    if (node && !add_child(dir, node)) {
        kfree(node);                // directory full
        return 0;
    }
    if (node)
        dir_changed(dir);
    return node;
}

int ramdisk_delete(vfs_node_t *dir, const char *name) {
    vfs_node_t *entry = ours(dir) ? ramdisk_finddir(dir, name) : 0;
    if (!entry || (entry->flags & FS_DIRECTORY && !ours(entry)))
        return -1;                  // missing, or another driver's directory

    // DEFENCE busy_check: not while open, not a non-empty directory,
    // not anyone's current directory.
    if (!def_delete_allowed(entry))
        return -1;

    remove_child(dir, entry);
    dir_changed(dir);

    // A file: one name less. Its bytes go only when no name and no
    // descriptor refers to it any more (DEFENCE inode_refcount).
    if (entry->flags & FS_FILE) {
        vfs_node_t *inode = vfs_inode(entry);
        if (inode->nlink > 0)
            inode->nlink--;
        inode->ctime = vfs_now();
        if (entry != inode)
            kfree(entry);           // a hard link entry: only a name
        else
            entry->parent = 0;      // the inode's own name is gone
        if (!inode->parent && def_inode_free_ok(inode))
            ramdisk_release(inode);
        return 0;
    }
    kfree(entry->data);             // a symbolic link's target text; NULL for a directory
    kfree(entry);
    return 0;
}

vfs_node_t *ramdisk_link(vfs_node_t *dir, const char *name, vfs_node_t *inode) {
    inode = vfs_inode(inode);
    if (!inode || !(inode->flags & FS_FILE) || !ours(inode->parent ? inode->parent : dir))
        return 0;
    // DEFENCE nlink_bounds: the count can never wrap around.
    if (!def_nlink_ok(inode->nlink))
        return 0;
    vfs_node_t *entry = ramdisk_create(dir, name, FS_FILE);
    if (!entry)
        return 0;                   // a bad name, taken, too deep, too long
    entry->read = entry->write = 0;
    entry->release = 0;
    entry->target = inode;          // this name means that file
    inode->nlink++;
    inode->ctime = vfs_now();
    return entry;
}

vfs_node_t *ramdisk_symlink(vfs_node_t *dir, const char *name, const char *target) {
    uint32_t len = strlen(target);
    if (!len || len >= FS_PATH_MAX)
        return 0;
    vfs_node_t *link = ramdisk_create(dir, name, FS_LINK);
    if (!link)
        return 0;
    link->data = kmalloc(len);
    if (!link->data) {
        ramdisk_delete(dir, name);
        return 0;
    }
    memcpy(link->data, target, len);
    link->size = link->capacity = len;
    link->mode = 066;               // a link's own mode is never used
    return link;
}

int ramdisk_move(vfs_node_t *node, vfs_node_t *new_dir, const char *new_name) {
    if (!node || node == fs_root || !ours(new_dir) || !ours(node->parent))
        return -1;                  // only between the RAM disk's own directories
    if (node->flags & FS_DIRECTORY && !ours(node))
        return -1;                  // another driver's directory (/proc) stays put

    // DEFENCE filename_check: the new name follows the same rules as a new file.
    if (!def_new_name_ok(new_name))
        return -1;

    // DEFENCE no_cycle: a directory cannot go inside itself or its children.
    if (!def_move_allowed(node, new_dir))
        return -1;

    // DEFENCE path_length + depth_limit: the whole moved subtree must still fit.
    if (!def_path_fits(new_dir, new_name, longest_below(node)) ||
        !def_depth_ok(new_dir, 1 + height_below(node)))
        return -1;

    vfs_node_t *existing = ramdisk_finddir(new_dir, new_name);
    if (existing && existing != node)
        return -1;                  // would overwrite another entry

    vfs_node_t *old_dir = node->parent;
    if (new_dir != old_dir) {
        if (new_dir->num_children >= FS_MAX_CHILDREN)
            return -1;
        remove_child(old_dir, node);
        add_child(new_dir, node);
    }
    strcpy(node->name, new_name);
    node->ctime = vfs_now();
    dir_changed(old_dir);
    dir_changed(new_dir);
    return 0;
}

// Where is 'node' in its directory's list?
static int slot_of(const vfs_node_t *dir, const vfs_node_t *node) {
    for (int i = 0; i < dir->num_children; i++)
        if (dir->children[i] == node)
            return i;
    return -1;
}

int ramdisk_exchange(vfs_node_t *a, vfs_node_t *b) {
    if (!a || !b || a == b || a == fs_root || b == fs_root ||
        !ours(a->parent) || !ours(b->parent))
        return -1;
    if (((a->flags & FS_DIRECTORY) && !ours(a)) || ((b->flags & FS_DIRECTORY) && !ours(b)))
        return -1;
    // DEFENCE no_cycle: neither may end up inside itself.
    if (vfs_is_ancestor(a, b) || vfs_is_ancestor(b, a))
        return -1;
    // DEFENCE path_length + depth_limit, for both new places.
    if (!def_path_fits(b->parent, a->name, longest_below(a)) ||
        !def_path_fits(a->parent, b->name, longest_below(b)) ||
        !def_depth_ok(b->parent, 1 + height_below(a)) ||
        !def_depth_ok(a->parent, 1 + height_below(b)))
        return -1;

    vfs_node_t *dir_a = a->parent, *dir_b = b->parent;
    int slot_a = slot_of(dir_a, a), slot_b = slot_of(dir_b, b);
    if (slot_a < 0 || slot_b < 0)
        return -1;
    char name[FS_NAME_MAX + 1];
    strcpy(name, a->name);
    strcpy(a->name, b->name);
    strcpy(b->name, name);
    dir_a->children[slot_a] = b;                // both slots found first: this works
    dir_b->children[slot_b] = a;                // in one directory too
    a->parent = dir_b;
    b->parent = dir_a;
    a->ctime = b->ctime = vfs_now();
    dir_changed(dir_a);
    dir_changed(dir_b);
    return 0;
}

void ramdisk_truncate(vfs_node_t *file) {
    if (file && (file->flags & FS_FILE)) {
        file->size = 0;             // keep the buffer; the next write reuses it
        file->mtime = file->ctime = vfs_now();
    }
}

int ramdisk_resize(vfs_node_t *file, uint32_t size) {
    if (!file || !(file->flags & FS_FILE) || !ours(file->parent) || size > RAMDISK_MAX_FILE_SIZE)
        return -1;
    if (size > file->size) {
        if (!grow(file, size))
            return -1;              // out of memory
        memset(file->data + file->size, 0, size - file->size);   // new bytes read as 0
    }
    file->size = size;
    file->mtime = file->ctime = vfs_now();
    return 0;
}

void ramdisk_init(void) {
    fs_root = new_node("/", FS_DIRECTORY);
    if (!fs_root)
        return;
    fs_root->parent = fs_root;      // ".." at the root stays at the root

    vfs_node_t *hello = ramdisk_create(fs_root, "hello", FS_FILE);
    if (hello)
        ramdisk_write(hello, 0, userland_hello_elf_len, (uint8_t *)userland_hello_elf);
}
