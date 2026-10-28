#ifndef DEF_FAIR_SCHEDULING_H
#define DEF_FAIR_SCHEDULING_H

// 1 if the scheduler is fair: a ready process that has waited long
// enough is eventually chosen no matter what else wants the CPU. The
// rule itself is aging (security/starvation/defence/aging), called by
// the scheduler on every decision; this only reports that it holds.
int def_fair_scheduling_ok(void);

#endif
