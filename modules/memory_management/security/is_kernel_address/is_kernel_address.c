#include "mem_security.h"

// Check if a virtual address lies within kernel space
int mem_is_kernel_address(uint32_t virt_addr) {
    return (virt_addr >= KERNEL_SPACE_START);
}
