#ifndef DEF_PROCESS_ISOLATION_H
#define DEF_PROCESS_ISOLATION_H

// 1 marks the isolation UzOS does provide: ring 3 code runs with only
// its own user pages mapped and cannot read or write kernel memory (the
// PAGE_USER bit, enforced by the CPU). Full inter-process memory
// isolation needs a separate page directory per process, which UzOS's
// cooperative kernel-mode tasks do not have - so this is documented, not
// a complete barrier.
int def_ring3_isolated(void);

#endif
