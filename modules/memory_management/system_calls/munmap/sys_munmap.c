#include "mem_syscalls.h"
#include "pmm.h"
#include "vmm.h"

// System Call: sys_munmap
// Unmaps user memory pages
int sys_munmap(void *addr, size_t length) {
    if (!addr || length == 0) return -1;

    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    pmm_free_blocks(addr, pages);
    return 0;
}
