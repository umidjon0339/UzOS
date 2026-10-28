/*
 * ramdisk.c - Store files and directories in the kernel heap.
 *
 * Every new node belongs to the user who created it (process_caller_uid)
 * and starts with the default mode (user_management/core/perm).
 *
 * Uses: kmalloc/kfree (memory_management), memcpy/strcmp (lib/string),
 *       and four defences:
 *         filename_check - names for new or renamed entries
 *         file_range     - offset + size never overflows or passes the limit
 *         busy_check     - never delete what someone is still using
 *         no_cycle       - never move a directory inside itself
 */

#include "ramdisk.h"
#include "kheap.h"
#include "string.h"
#include "hello_elf.h"
#include "process.h"
#include "perm.h"
#include "def_filename_check.h"
#include "def_file_range.h"
#include "def_busy_check.h"
#include "def_no_cycle.h"

#define FIRST_CAPACITY 256          // a new file starts with 256 bytes of room

/* ---------- Driver functions (called through the VFS) ---------- */

static uint32_t ramdisk_read(vfs_node_t *file, uint32_t offset, uint32_t size, uint8_t *buf) {
    if (!(file->flags & FS_FILE) || !buf || offset >= file->size)
        return 0;
    if (size > file->size - offset)
        size = file->size - offset;     // stop at the end of the file

    memcpy(buf, file->data + offset, size);
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

static vfs_node_t *new_node(const char *name, uint32_t type) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node)
        return 0;
    memset(node, 0, sizeof(*node));
    strcpy(node->name, name);       // length already checked by filename_check
    node->flags = type;
    node->owner = process_caller_uid();     // whoever creates it owns it
    node->mode  = MODE_DEFAULT;             // user_management/core/perm

    if (type == FS_FILE) {
        node->read  = ramdisk_read;
        node->write = ramdisk_write;
    } else {
        node->readdir = ramdisk_readdir;
        node->finddir = ramdisk_finddir;
    }
    return node;
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

    vfs_node_t *node = new_node(name, type);
    if (node && !add_child(dir, node)) {
        kfree(node);                // directory full
        return 0;
    }
    return node;
}

int ramdisk_delete(vfs_node_t *dir, const char *name) {
    vfs_node_t *target = ours(dir) ? ramdisk_finddir(dir, name) : 0;
    if (!target || (target->flags & FS_DIRECTORY && !ours(target)))
        return -1;                  // missing, or another driver's directory

    // DEFENCE busy_check: not while open, not a non-empty directory,
    // not anyone's current directory.
    if (!def_delete_allowed(target))
        return -1;

    remove_child(dir, target);
    kfree(target->data);            // kfree(NULL) is fine for directories
    kfree(target);
    return 0;
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
    return 0;
}

void ramdisk_truncate(vfs_node_t *file) {
    if (file && (file->flags & FS_FILE))
        file->size = 0;             // keep the buffer; the next write reuses it
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
