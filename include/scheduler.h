#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_init(void);
void scheduler_start(void);
void scheduler_tick(void); // Called from the PIT timer interrupt
void scheduler_yield(void);
void scheduler_set_current_process(uint32_t pid);
process_t *scheduler_get_current_process(void);

extern void switch_task(uint32_t *old_esp, uint32_t new_esp);

#endif
