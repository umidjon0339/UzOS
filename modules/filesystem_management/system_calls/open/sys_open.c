#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "fs_security.h"
#include "vfs.h"
#include "scheduler.h"

int sys_open(const char *path) {
    process_t *current = scheduler_get_current_process();
    if (!current || !fs_security_check_path(path)) return -1;
    
    // For now, our paths are extremely simple. We just look in root (fs_root).
    // In a real OS we would parse the path like "/dir/file".
    // Let's strip the leading slash if it exists.
    if (path[0] == '/') path++;
    
    if (path[0] == '\0') return -1;
    
    vfs_node_t *dir = current->cwd;
    if (!dir) dir = fs_root;
    
    vfs_node_t *node = vfs_finddir(dir, path);
    
    // If it doesn't exist, we don't create it here. 
    // They should use a separate "create" syscall or a flag.
    if (!node) return -1;
    
    int fd = get_free_fd(current);
    if (fd == -1) return -1;
    
    vfs_open(node);
    
    current->fd_table[fd] = node;
    current->fd_offset[fd] = 0;
    
    return fd;
}
