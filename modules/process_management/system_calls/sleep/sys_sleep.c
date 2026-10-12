#include "proc_syscalls.h"
#include "scheduler.h"
#include "process.h"

void sys_sleep(uint32_t ms) {
    uint32_t ticks = ms / 10;
    if (ticks == 0) ticks = 1; // Sleep at least 1 tick
    
    process_t *current = scheduler_get_current_process();
    if (current) {
        current->state = PROCESS_STATE_SLEEPING;
        current->sleep_ticks = ticks;
        scheduler_yield();
    }
}
