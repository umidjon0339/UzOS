#include "../fs_syscalls_internal.h"

int get_free_fd(process_t *proc) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (proc->fd_table[i] == 0) return i;
    }
    return -1;
}
