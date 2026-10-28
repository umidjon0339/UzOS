#ifndef DEF_AGING_H
#define DEF_AGING_H

#include <stdint.h>

// Waiting this many ticks raises a process by one level (100 ms at 100 Hz).
#define AGING_TICKS 10

// Base priority 'base' after waiting 'waited' ticks: never above
// PRIORITY_MAX, and the idle task is never raised.
int def_aging_effective(int base, uint32_t waited);

// The scheduler picked a process only because it had aged: log it
// (at most once a second, so a long starvation does not flood the log).
void def_aging_note_rescue(void);

#endif
