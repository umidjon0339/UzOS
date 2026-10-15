#include "mem_syscalls.h"
#include "mem_security.h"
#include "pmm.h"

// System Call: sys_munmap
// Unmaps user memory pages
int sys_munmap(void *addr, size_t length) {
    size_t pages;
    if (!mem_validate_user_ptr(addr, length) ||
        !mem_mmap_page_count(length, &pages) ||
        !mem_pmm_range_valid(addr, pages)) return -1;

    pmm_free_blocks(addr, pages);
    return 0;
}
