#include "mem_syscalls.h"
#include "kheap.h"

// System Call: sys_sbrk
// Increases or decreases program break / heap limit
void *sys_sbrk(int incr) {
    if (incr == 0) {
        return (void *)KHEAP_START;
    }
    if (incr > 0) {
        return kmalloc((size_t)incr);
    }
    return NULL;
}
