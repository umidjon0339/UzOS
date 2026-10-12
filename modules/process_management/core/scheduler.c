#include "scheduler.h"
#include "process.h"
#include <stddef.h>

static process_t *current_process = NULL;
static int current_process_index = -1;
static int scheduler_enabled = 0;

static void idle_worker(void) {
    while (1) {
        asm volatile("hlt");
    }
}

void scheduler_init(void) {
    process_init();
    
    // Spawn the idle task (PID 1 typically, but kernel_main might be PID 1)
    process_create("idle", idle_worker);
    
    scheduler_enabled = 0;
}

void scheduler_start(void) {
    scheduler_enabled = 1;
}

void scheduler_set_current_process(uint32_t pid) {
    process_t *ptable = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (ptable[i].pid == pid && ptable[i].state != PROCESS_STATE_FREE) {
            current_process = &ptable[i];
            current_process_index = i;
            return;
        }
    }
}

process_t *scheduler_get_current_process(void) {
    return current_process;
}

static int get_next_ready_process(int start_idx) {
    process_t *ptable = process_get_table();
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (start_idx + i) % MAX_PROCESSES;
        if (ptable[idx].state == PROCESS_STATE_READY || ptable[idx].state == PROCESS_STATE_RUNNING) {
            return idx;
        }
    }
    return -1;
}

void scheduler_tick(void) {
    if (!scheduler_enabled) return;

    process_t *ptable = process_get_table();
    
    // Decrement sleep ticks for sleeping processes
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (ptable[i].state == PROCESS_STATE_SLEEPING) {
            if (ptable[i].sleep_ticks > 0) {
                ptable[i].sleep_ticks--;
            }
            if (ptable[i].sleep_ticks == 0) {
                ptable[i].state = PROCESS_STATE_READY;
            }
        }
    }
    
    scheduler_yield();
}

void scheduler_yield(void) {
    if (!scheduler_enabled) return;
    
    process_t *ptable = process_get_table();
    
    int next_idx = get_next_ready_process(current_process_index != -1 ? current_process_index : 0);
    if (next_idx == -1) {
        // This should theoretically never happen now because the idle task is always READY.
        return;
    }
    
    if (next_idx == current_process_index) {
        // Only one process, no need to switch
        return;
    }
    
    process_t *prev_process = current_process;
    process_t *next_process = &ptable[next_idx];
    
    if (prev_process && prev_process->state == PROCESS_STATE_RUNNING) {
        prev_process->state = PROCESS_STATE_READY;
    }
    
    next_process->state = PROCESS_STATE_RUNNING;
    
    current_process = next_process;
    current_process_index = next_idx;
    
    if (prev_process) {
        switch_task(&prev_process->esp, next_process->esp);
    } else {
        // First ever task switch, we just need a dummy place to store old esp
        // so we don't crash.
        uint32_t dummy_esp;
        switch_task(&dummy_esp, next_process->esp);
    }
}
