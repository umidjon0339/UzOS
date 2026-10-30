#ifndef DEF_PCB_WIPE_H
#define DEF_PCB_WIPE_H

#include "modules/process_management/core/process/process.h"  // process_t

// Zero every field of a slot and mark it FREE.
void def_pcb_wipe(process_t *p);

#endif
