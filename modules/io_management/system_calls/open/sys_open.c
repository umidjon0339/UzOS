#include "io_syscalls.h"
#include "io_security.h"
#include "../../core/io_internal.h"
#include "fs_security.h"
#include "scheduler.h"
#include "vfs.h"

int sys_open(const char *path) {
    process_t *current = scheduler_get_current_process();
    if (!current || !fs_security_check_path(path)) return -1;

    const char *name = path[0] == '/' ? path + 1 : path;
    vfs_node_t *dir = current->cwd ? current->cwd : fs_root;
    if (!dir) return -1;
    vfs_node_t *node = vfs_finddir(dir, name);
    if (!node) return -1;

    int fd = io_find_free_file_descriptor(current);
    if (fd < 0) return -1;

    vfs_open(node);
    current->fd_table[fd] = node;
    current->fd_offset[fd] = 0;
    return fd;
}
