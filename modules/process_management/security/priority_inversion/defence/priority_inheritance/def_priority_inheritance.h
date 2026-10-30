#ifndef DEF_PRIORITY_INHERITANCE_H
#define DEF_PRIORITY_INHERITANCE_H

// The priority a lock HOLDER runs at while a waiter is blocked on its
// lock: the higher of the two (levels as core/priority: 1 low .. 7 high).
// CONCEPT: UzOS has no locks user programs share (no futex), so nothing
// calls this yet; it is the rule a futex / mutex would apply.
int def_pi_priority(int holder, int waiter);

#endif
