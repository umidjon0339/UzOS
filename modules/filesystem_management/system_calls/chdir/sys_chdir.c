#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "scheduler.h"
#include "string.h"

int sys_chdir(const char *path) {
    process_t *current = scheduler_get_current_process();
    if (!current) return -1;
    
    if (path[0] == '\0') {
        current->cwd = fs_root;
        return 0;
    }
    
    vfs_node_t *dir = current->cwd ? current->cwd : fs_root;
    
    if (strcmp(path, "/") == 0) {
        current->cwd = fs_root;
        return 0;
    }
    
    if (strcmp(path, "..") == 0) {
        current->cwd = dir->parent;
        return 0;
    }
    
    if (strcmp(path, ".") == 0) {
        return 0;
    }
    
    vfs_node_t *target = vfs_finddir(dir, path);
    if (!target || target->flags != FS_DIRECTORY) return -1;
    
    current->cwd = target;
    return 0;
}
