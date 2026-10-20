/*
 * vfs.c - Node operations, open counts, and walking paths.
 *
 * Uses: the driver's function pointers (core/ramdisk), the current
 *       process (process_management) for its working directory,
 *       cpu_irq_save (arch/x86/cpu), and the defence filename_check for
 *       every path component.
 */

#include "vfs.h"
#include "scheduler.h"
#include "string.h"
#include "cpu.h"
#include "def_filename_check.h"

vfs_node_t *fs_root;

/* ---------- Using a node ---------- */

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    return node && node->read ? node->read(node, offset, size, buf) : 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    return node && node->write ? node->write(node, offset, size, buf) : 0;
}

vfs_node_t *vfs_readdir(vfs_node_t *dir, uint32_t index) {
    if (!dir || !(dir->flags & FS_DIRECTORY) || !dir->readdir)
        return 0;
    return dir->readdir(dir, index);
}

vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name) {
    if (!dir || !(dir->flags & FS_DIRECTORY) || !dir->finddir)
        return 0;
    return dir->finddir(dir, name);
}

// Interrupts off: the reaper closes files from the timer interrupt, and
// "count = count + 1" is a read and a write that must not be split.
void vfs_open(vfs_node_t *node) {
    uint32_t irq = cpu_irq_save();
    if (node)
        node->open_count++;
    cpu_irq_restore(irq);
}

void vfs_close(vfs_node_t *node) {
    uint32_t irq = cpu_irq_save();
    if (node && node->open_count > 0)
        node->open_count--;
    cpu_irq_restore(irq);
}

/* ---------- Paths ---------- */

vfs_node_t *vfs_cwd(void) {
    process_t *self = scheduler_get_current_process();
    return self && self->cwd ? self->cwd : fs_root;
}

// Copy the next component of *path into 'name' and move *path past it.
// Returns the component's length, 0 at the end, or -1 if it is too long.
static int next_component(const char **path, char *name) {
    const char *p = *path;
    while (*p == '/')
        p++;                                // "a//b" is the same as "a/b"

    int len = 0;
    while (p[len] && p[len] != '/')
        len++;

    // DEFENCE filename_check: never copy a name longer than the buffer.
    if (len && !def_component_ok(len))
        return -1;

    memcpy(name, p, len);
    name[len] = '\0';
    *path = p + len;
    return len;
}

// Take one step from 'dir' by 'name' ("." stays, ".." goes up).
static vfs_node_t *step(vfs_node_t *dir, const char *name) {
    if (strcmp(name, ".") == 0)
        return dir;
    if (strcmp(name, "..") == 0)
        return dir->parent;                 // the root's parent is the root
    return vfs_finddir(dir, name);
}

vfs_node_t *vfs_lookup(vfs_node_t *start, const char *path) {
    if (!path || !fs_root)
        return 0;

    vfs_node_t *node = (path[0] == '/' || !start) ? fs_root : start;
    char name[FS_NAME_MAX + 1];
    int len;

    while ((len = next_component(&path, name)) > 0) {
        if (!(node->flags & FS_DIRECTORY))
            return 0;                       // "file/x": a file has no children
        node = step(node, name);
        if (!node)
            return 0;
    }
    return len < 0 ? 0 : node;
}

vfs_node_t *vfs_lookup_parent(vfs_node_t *start, const char *path, char *name_out) {
    if (!path)
        return 0;

    // Split "a/b/c" into the directory part "a/b" and the last name "c".
    int end = strlen(path);
    while (end > 0 && path[end - 1] == '/')
        end--;                              // ignore trailing slashes: "a/b/"
    int last = end;
    while (last > 0 && path[last - 1] != '/')
        last--;

    int name_len = end - last;
    if (name_len == 0 || !def_component_ok(name_len))
        return 0;
    memcpy(name_out, path + last, name_len);
    name_out[name_len] = '\0';

    // The directory part, as its own string ("/" if it was "/c").
    char dir_path[FS_PATH_MAX];
    if (last >= FS_PATH_MAX)
        return 0;
    memcpy(dir_path, path, last);
    dir_path[last] = '\0';

    vfs_node_t *dir = vfs_lookup(path[0] == '/' ? fs_root : start, dir_path);
    return (dir && (dir->flags & FS_DIRECTORY)) ? dir : 0;
}

int vfs_path_of(vfs_node_t *node, char *buf, int max) {
    if (!node || max < 2)
        return -1;
    if (node == fs_root) {
        strcpy(buf, "/");
        return 0;
    }

    // Build the path backwards, from the node up to the root.
    char tmp[FS_PATH_MAX];
    int pos = FS_PATH_MAX - 1;
    tmp[pos] = '\0';

    for (; node && node != fs_root; node = node->parent) {
        int len = strlen(node->name);
        if (pos - len - 1 < 0)
            return -1;
        pos -= len;
        memcpy(&tmp[pos], node->name, len);
        tmp[--pos] = '/';
    }

    int total = FS_PATH_MAX - 1 - pos;
    if (total + 1 > max)
        return -1;
    memcpy(buf, &tmp[pos], total + 1);
    return 0;
}

int vfs_is_ancestor(vfs_node_t *ancestor, vfs_node_t *node) {
    while (node) {
        if (node == ancestor)
            return 1;
        if (node == node->parent)
            return 0;                       // reached the root
        node = node->parent;
    }
    return 0;
}
