#include "../fs_syscalls_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "scheduler.h"
#include "string.h"

struct vfs_node *sys_getcwd_node(void) {
    process_t *current = scheduler_get_current_process();
    if (!current) return fs_root;
    return current->cwd ? current->cwd : fs_root;
}

int sys_getcwd_string(char *buf, int max_len) {
    vfs_node_t *curr = sys_getcwd_node();
    if (curr == fs_root) {
        if (max_len > 1) {
            buf[0] = '/';
            buf[1] = '\0';
            return 0;
        }
        return -1;
    }
    
    char path[256] = {0};
    int p = 255;
    while (curr != fs_root && curr != 0) {
        int len = strlen(curr->name);
        p -= len;
        if (p < 0) return -1;
        memcpy(&path[p], curr->name, len);
        p--;
        if (p < 0) return -1;
        path[p] = '/';
        curr = curr->parent;
    }
    
    int final_len = 255 - p;
    if (final_len >= max_len) return -1;
    
    memcpy(buf, &path[p], final_len);
    buf[final_len] = '\0';
    return 0;
}
