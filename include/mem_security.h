#ifndef MEM_SECURITY_H
#define MEM_SECURITY_H

#include <stdint.h>
#include <stddef.h>

#define KERNEL_SPACE_START 0xC0000000 // 3 GB higher-half boundary

int mem_validate_user_ptr(const void *ptr, size_t size);
int mem_is_kernel_address(uint32_t virt_addr);
int mem_user_range_valid(uint32_t start, size_t size);
int mem_mmap_page_count(size_t length, size_t *pages);
int mem_pmm_range_valid(const void *addr, size_t count);

#endif
