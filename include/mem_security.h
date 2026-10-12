#ifndef MEM_SECURITY_H
#define MEM_SECURITY_H

#include <stdint.h>
#include <stddef.h>

#define KERNEL_SPACE_START 0xC0000000 // 3 GB higher-half boundary

int mem_validate_user_ptr(const void *ptr, size_t size);
int mem_is_kernel_address(uint32_t virt_addr);

#endif
