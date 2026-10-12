#include "proc_syscalls.h"
#include "scheduler.h"

void sys_exit(void) {
    // Calls process_kill(current_pid) essentially, then yield.
    // We will implement this fully in Phase 8 (User mode syscalls).
    while(1) {
        scheduler_yield();
    }
}
