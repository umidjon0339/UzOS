#include "mem_security.h"
#include "proc_security.h"

int mem_user_range_valid(uint32_t start, size_t size) {
    if (start >= KERNEL_SPACE_START) return 0;
    return size <= (size_t)(KERNEL_SPACE_START - start);
}

int mem_validate_user_ptr(const void *ptr, size_t size) {
    if (!ptr) return 0;
    return mem_user_range_valid((uint32_t)ptr, size);
}

int mem_is_kernel_address(uint32_t virt_addr) {
    return virt_addr >= KERNEL_SPACE_START;
}

static int def_user_ptr_bounds_selftest(void) {
    if (!mem_user_range_valid(0x1000u, 1)) return 0;
    if (!mem_user_range_valid(KERNEL_SPACE_START - 1, 1)) return 0;
    if (mem_user_range_valid(KERNEL_SPACE_START - 1, 2)) return 0;
    if (mem_user_range_valid(0xFFFFFFFFu, 2)) return 0;
    if (mem_validate_user_ptr(0, 1)) return 0;
    return 1;
}

static const defence_t def = {
    "user_ptr_bounds",
    "reject null, wrapped, or kernel-crossing user memory ranges",
    "user_ptr_wrap",
    def_user_ptr_bounds_selftest
};

void def_user_ptr_bounds_register(void) {
    proc_security_register_defence(&def);
}
