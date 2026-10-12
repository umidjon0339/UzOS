#include "mem_security.h"

// Validate that a user-mode buffer pointer is strictly within user space
int mem_validate_user_ptr(const void *ptr, size_t size) {
    if (!ptr) return 0; // Null pointer invalid

    uint32_t start = (uint32_t)ptr;
    uint32_t end   = start + size;

    // Integer overflow protection check
    if (end < start) return 0;

    // Ensure pointer range doesn't cross into Kernel Space
    if (start >= KERNEL_SPACE_START || end > KERNEL_SPACE_START) {
        return 0; // Security violation!
    }

    return 1; // Valid user pointer
}
