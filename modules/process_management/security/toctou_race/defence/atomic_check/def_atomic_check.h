#ifndef DEF_ATOMIC_CHECK_H
#define DEF_ATOMIC_CHECK_H

// A TOCTOU bug is a gap between CHECKING a condition and USING the
// result: something changes in between. The fix is to check and use in
// ONE step that cannot be split. 1 if an operation done in
// 'single_step' form is safe (no gap); 0 if it is a separate
// check-then-use (a race window).
int def_check_is_atomic(int single_step);

#endif
