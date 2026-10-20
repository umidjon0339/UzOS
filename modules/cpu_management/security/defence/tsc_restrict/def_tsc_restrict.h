#ifndef DEF_TSC_RESTRICT_H
#define DEF_TSC_RESTRICT_H

#include <stdint.h>

// Set CR4.TSD: from now on rdtsc in ring 3 raises exception 13.
void def_tsc_restrict_enable(void);

// Called for every ring 3 fault: if it was an rdtsc, log the refusal.
void def_tsc_restrict_note(uint8_t vector, const char *instruction);

#endif
