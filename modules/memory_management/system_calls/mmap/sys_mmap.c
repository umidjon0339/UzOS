#include "mem_syscalls.h"
#include "vmm.h"
#include "pmm.h"

// System Call: sys_mmap
// Allocates and maps contiguous virtual memory pages for process space
void *sys_mmap(void *addr, size_t length, int prot, int flags) {
    (void)addr;
    (void)prot;
    (void)flags;

    if (length == 0) return NULL;

    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    void *phys = pmm_alloc_blocks(pages);
    if (!phys) return NULL;

    // Return allocated physical memory pointer (identity mapped)
    return phys;
}
