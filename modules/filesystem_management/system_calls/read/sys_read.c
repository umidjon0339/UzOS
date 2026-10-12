#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "scheduler.h"

int sys_read(int fd, void *buf, uint32_t count) {
    process_t *current = scheduler_get_current_process();
    if (!current || fd < 0 || fd >= MAX_FDS) return -1;
    
    vfs_node_t *node = current->fd_table[fd];
    if (!node) return -1;
    
    uint32_t bytes_read = vfs_read(node, current->fd_offset[fd], count, (uint8_t *)buf);
    current->fd_offset[fd] += bytes_read;
    return bytes_read;
}
