#include "proc_syscalls.h"
#include "scheduler.h"

void sys_yield(void) {
    // In user mode, this would trigger an int 0x80.
    // For now, in ring 0, we can just call scheduler_yield() directly.
    scheduler_yield();
}
