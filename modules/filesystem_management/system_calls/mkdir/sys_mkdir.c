#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "scheduler.h"
#include "ramdisk.h"

int sys_mkdir(const char *path) {
    if (path[0] == '/') path++;
    if (path[0] == '\0') return -1;
    
    process_t *current = scheduler_get_current_process();
    vfs_node_t *dir = current && current->cwd ? current->cwd : fs_root;
    
    vfs_node_t *new_dir = ramdisk_create_dir(dir, path);
    if (!new_dir) return -1;
    return 0;
}
