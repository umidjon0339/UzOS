/*
 * procfs.c - The /proc driver: directories and files made on demand.
 *
 * A VFS node is just a name plus function pointers (read, readdir,
 * finddir). The RAM disk's nodes store bytes; ours compute them:
 *
 *   read     run the file's generator into a buffer, copy the asked part
 *   readdir  /proc: cpuinfo, stat, self, then one entry per process
 *   finddir  /proc/"7": is there a PID 7? then the node of its slot
 *
 * A file is generated again when a read starts at offset 0, so every
 * `cat` sees fresh numbers, and the later chunks of the same `cat`
 * come from the same snapshot.
 *
 * Uses: kmalloc (memory_management) for the nodes, fs_root and FS_*
 *       (filesystem_management), the process table and the current
 *       process (process_management), vformat (lib/format), and the
 *       three generators next to it: proc_cpuinfo.c, proc_stat.c,
 *       proc_pid_status.c.
 */

#include "modules/filesystem_management/core/procfs/procfs.h"              // this part
#include "modules/filesystem_management/core/procfs/proc_cpuinfo.h"       // proc_cpuinfo
#include "modules/filesystem_management/core/procfs/proc_stat.h"             // proc_stat
#include "modules/filesystem_management/core/procfs/proc_pid_status.h" // proc_pid_status
#include "modules/memory_management/core/kheap/kheap.h"             // kmalloc
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "lib/format/format.h"                                      // vformat, format
#include "lib/string/string.h"                                      // memset, strcpy
#include <stdarg.h>

#define MODE_READ_ONLY 044      // owner r, others r (user_management/core/perm)
#define SELF_SLOT MAX_PROCESSES // the "self" directory sits after the process slots

static vfs_node_t *proc_dir, *cpuinfo_file, *stat_file;
static vfs_node_t *pid_dirs[MAX_PROCESSES + 1];     // [slot] = /proc/<pid>, [SELF_SLOT] = self
static vfs_node_t *status_files[MAX_PROCESSES + 1]; // [slot] = /proc/<pid>/status
static uint32_t    dir_pid[MAX_PROCESSES + 1];      // which pid each directory shows now

// One snapshot of the file being read (see the top of this file).
static char snapshot[PROC_TEXT_MAX];
static int  snapshot_length;
static vfs_node_t *snapshot_of;

void proc_printf(proc_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf->length += vformat(buf->text + buf->length, buf->size - buf->length, fmt, args);
    va_end(args);
}

// Which slot index a pid directory or status file stands for.
static int index_of(vfs_node_t *node, vfs_node_t **list) {
    for (int i = 0; i <= MAX_PROCESSES; i++)
        if (list[i] == node)
            return i;
    return -1;
}

// The process a /proc/<pid> (or self) directory shows: NULL if it is gone.
static process_t *process_of_dir(int index) {
    if (index == SELF_SLOT)
        return scheduler_get_current_process();
    return process_get_by_pid(dir_pid[index]);
}

static void generate(vfs_node_t *file) {
    proc_buf_t buf = { snapshot, 0, sizeof(snapshot) };
    snapshot[0] = '\0';

    if (file == cpuinfo_file) {
        proc_cpuinfo(&buf);
    } else if (file == stat_file) {
        proc_stat(&buf);
    } else {
        int index = index_of(file, status_files);
        process_t *p = index >= 0 ? process_of_dir(index) : 0;
        if (p)
            proc_pid_status(&buf, p);
    }
    snapshot_length = buf.length;
    snapshot_of = file;
}

static uint32_t proc_read(vfs_node_t *file, uint32_t offset, uint32_t size, uint8_t *out) {
    if (offset == 0 || snapshot_of != file)
        generate(file);
    if (offset >= (uint32_t)snapshot_length)
        return 0;
    if (size > snapshot_length - offset)
        size = snapshot_length - offset;
    memcpy(out, snapshot + offset, size);
    return size;
}

/* ---------- Directories ---------- */

// Point pid directory 'index' at process 'p' (its name becomes the pid).
static vfs_node_t *show_process(int index, const process_t *p) {
    dir_pid[index] = p->pid;
    format(pid_dirs[index]->name, FS_NAME_MAX, "%u", p->pid);
    pid_dirs[index]->owner = p->uid;
    status_files[index]->owner = p->uid;
    return pid_dirs[index];
}

static vfs_node_t *proc_root_readdir(vfs_node_t *dir, uint32_t index) {
    (void)dir;
    if (index == 0) return cpuinfo_file;
    if (index == 1) return stat_file;
    if (index == 2) return pid_dirs[SELF_SLOT];

    // Then the (index - 3)-th process that exists.
    process_t *table = process_get_table();
    uint32_t n = 3;
    for (int slot = 0; slot < MAX_PROCESSES; slot++) {
        if (table[slot].state == PROCESS_STATE_FREE)
            continue;
        if (n++ == index)
            return show_process(slot, &table[slot]);
    }
    return 0;
}

static vfs_node_t *proc_root_finddir(vfs_node_t *dir, const char *name) {
    (void)dir;
    if (strcmp(name, "cpuinfo") == 0) return cpuinfo_file;
    if (strcmp(name, "stat") == 0)    return stat_file;
    if (strcmp(name, "self") == 0)    return pid_dirs[SELF_SLOT];

    // A number: is there such a process?
    uint32_t pid = 0;
    for (const char *c = name; *c; c++) {
        if (*c < '0' || *c > '9' || pid > 100000000)
            return 0;
        pid = pid * 10 + (*c - '0');
    }
    process_t *p = process_get_by_pid(pid);
    return p ? show_process(p - process_get_table(), p) : 0;
}

static vfs_node_t *pid_dir_readdir(vfs_node_t *dir, uint32_t index) {
    int i = index_of(dir, pid_dirs);
    return (i >= 0 && index == 0 && process_of_dir(i)) ? status_files[i] : 0;
}

static vfs_node_t *pid_dir_finddir(vfs_node_t *dir, const char *name) {
    int i = index_of(dir, pid_dirs);
    if (i < 0 || strcmp(name, "status") != 0 || !process_of_dir(i))
        return 0;
    return status_files[i];
}

/* ---------- Making the nodes ---------- */

static vfs_node_t *new_node(const char *name, uint32_t type, vfs_node_t *parent) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node)
        return 0;
    memset(node, 0, sizeof(*node));
    strcpy(node->name, name);
    node->flags  = type;
    node->mode   = MODE_READ_ONLY;
    node->owner  = 0;
    node->parent = parent;
    if (type == FS_FILE)
        node->read = proc_read;         // no write: /proc files cannot be changed
    return node;
}

void procfs_init(void) {
    if (!fs_root || fs_root->num_children >= FS_MAX_CHILDREN)
        return;

    proc_dir = new_node("proc", FS_DIRECTORY, fs_root);
    if (!proc_dir)
        return;
    proc_dir->readdir = proc_root_readdir;
    proc_dir->finddir = proc_root_finddir;

    cpuinfo_file = new_node("cpuinfo", FS_FILE, proc_dir);
    stat_file    = new_node("stat", FS_FILE, proc_dir);

    for (int i = 0; i <= MAX_PROCESSES; i++) {
        pid_dirs[i] = new_node(i == SELF_SLOT ? "self" : "?", FS_DIRECTORY, proc_dir);
        if (!pid_dirs[i])
            return;
        pid_dirs[i]->readdir = pid_dir_readdir;
        pid_dirs[i]->finddir = pid_dir_finddir;
        status_files[i] = new_node("status", FS_FILE, pid_dirs[i]);
    }

    // Hang /proc into the root, as the RAM disk does with its own entries.
    fs_root->children[fs_root->num_children++] = proc_dir;
}
