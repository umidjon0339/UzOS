/*
 * def_user_ptr_bounds.c - DEFENCE user_ptr_bounds: pointers from ring 3.
 *
 * Stops:  user_ptr_wrap (security/attack/user_ptr_wrap)
 * Called: the int 0x80 handler (process_management), for every address a
 *         ring 3 program passes in: buffers (read, write, getcwd, munmap)
 *         and strings (paths for open, mkdir, ...).
 * Rules:  1. the whole range is inside user space
 *            (USER_SPACE_START <= start, size <= USER_SPACE_END - start)
 *         2. every page in it is mapped, with the USER bit
 *         A string is checked byte by byte the same way, up to its NUL.
 * Why:    "start + size" in 32 bits wraps: 0xFFFFFFF0 + 0x20 = 0x10, so
 *         a naive check passes a range that covers the kernel. And if a
 *         page is not mapped, the KERNEL page-faults while copying, which
 *         halts the whole OS. A program must only ever make itself fail.
 */

#include "def_user_ptr_bounds.h"
#include "registry.h"
#include "audit.h"
#include "memory_layout.h"
#include "vmm.h"

// Rule 1, as a pure function so the selftest can try it.
static int in_user_space(uint32_t start, uint32_t size) {
    if (start < USER_SPACE_START || start >= USER_SPACE_END) return 0;
    return size <= USER_SPACE_END - start;          // no addition: no wrap
}

// Rule 2: is the page holding 'addr' mapped for ring 3?
static int user_page_mapped(uint32_t addr) {
    vmm_walk_t w;
    return vmm_walk(addr, &w) && (w.pde & PAGE_USER) && (w.pte & PAGE_USER);
}

int def_user_range_ok(uint32_t start, uint32_t size) {
    const char *reason = 0;

    if (!in_user_space(start, size)) {
        reason = "address range outside user space";
    } else {
        // Every page from the first byte's to the last byte's. Rule 1
        // already made sure start + size stays below USER_SPACE_END.
        uint32_t first = start & ~(PAGE_SIZE - 1);
        uint32_t last  = (size ? start + size - 1 : start) & ~(PAGE_SIZE - 1);
        for (uint32_t page = first; page <= last; page += PAGE_SIZE) {
            if (!user_page_mapped(page)) {
                reason = "user buffer is not mapped";
                break;
            }
        }
    }

    if (!reason)
        return 1;
    audit_log("user_ptr_bounds", reason);
    return 0;
}

int def_user_string_ok(uint32_t start, uint32_t max) {
    for (uint32_t i = 0; i < max; i++) {
        uint32_t addr = start + i;
        int new_page = i == 0 || (addr & (PAGE_SIZE - 1)) == 0;

        if (new_page && (!in_user_space(addr, 1) || !user_page_mapped(addr))) {
            audit_log("user_ptr_bounds", "string is not in mapped user memory");
            return 0;
        }
        if (*(const char *)addr == '\0')
            return 1;                               // found the end in time
    }
    audit_log("user_ptr_bounds", "string has no end within the limit");
    return 0;
}

static int selftest(void) {
    if (!in_user_space(0x08048000, 0x1000)) return 0;           // an ELF page
    if (!in_user_space(USER_SPACE_END - 4, 4)) return 0;        // last bytes
    if (in_user_space(USER_SPACE_END - 4, 8)) return 0;         // crosses the end
    if (in_user_space(0x00100000, 16)) return 0;                // kernel code
    if (in_user_space(KHEAP_START, 16)) return 0;               // kernel heap
    if (in_user_space(0xFFFFF000, 0x2000)) return 0;            // wraps to 0x1000
    if (user_page_mapped(MMAP_END - PAGE_SIZE)) return 0;       // nothing mapped there
    return 1;
}

static const defence_t defence = {
    "user_ptr_bounds",
    "ring 3 pointers must be inside user space and mapped, without wrap",
    "user_ptr_wrap",
    selftest
};

void def_user_ptr_bounds_register(void) {
    registry_add_defence(&defence);
}
