#ifndef DEF_PMM_FREE_GUARD_H
#define DEF_PMM_FREE_GUARD_H

#include <stdint.h>

// May the frame at physical 'addr' be freed? 1 = yes, 0 = refused (logged).
int def_pmm_free_ok(uint32_t addr);

#endif
