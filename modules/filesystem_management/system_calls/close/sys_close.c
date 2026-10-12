#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "scheduler.h"

int sys_close(int fd) {
    process_t *current = scheduler_get_current_process();
    if (!current || fd < 0 || fd >= MAX_FDS) return -1;
    
    vfs_node_t *node = current->fd_table[fd];
    if (!node) return -1;
    
    vfs_close(node);
    current->fd_table[fd] = 0;
    current->fd_offset[fd] = 0;
    return 0;
}
