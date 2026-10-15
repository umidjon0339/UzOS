#include "mem_syscalls.h"
#include "mem_security.h"
#include "pmm.h"

// System Call: sys_mmap
// Allocates and maps contiguous virtual memory pages for process space
void *sys_mmap(void *addr, size_t length, int prot, int flags) {
    (void)addr;
    (void)prot;
    (void)flags;

    size_t pages;
    if (!mem_mmap_page_count(length, &pages)) return NULL;

    void *phys = pmm_alloc_blocks(pages);
    if (!phys) return NULL;

    // Return allocated physical memory pointer (identity mapped)
    return phys;
}
