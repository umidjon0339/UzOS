#ifndef DEF_PCB_GENERATION_H
#define DEF_PCB_GENERATION_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// A saved reference to ONE process: its slot, and which life of that
// slot it was (pid + generation). Keep this, not a bare process_t *,
// across anything that may sleep.
typedef struct {
    process_t *slot;
    uint32_t   pid;
    uint32_t   generation;
} pcb_ref_t;

// Remember 'p' (NULL gives an empty reference).
pcb_ref_t def_pcb_ref(process_t *p);

// The process 'ref' was made for, or NULL if it has been freed. If the
// slot now holds ANOTHER process, that is refused and logged: a stale
// pointer must never act on a newcomer. core/pidfd uses this.
process_t *def_pcb_ref_get(const pcb_ref_t *ref);

#endif
