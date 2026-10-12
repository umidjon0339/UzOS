#include "proc_syscalls.h"

uint32_t sys_getpid(void) {
    // We would need to retrieve current_process from scheduler.
    // However, scheduler current_process is internal to scheduler.c.
    // For now we don't expose it, just return 0 as dummy.
    return 0;
}
