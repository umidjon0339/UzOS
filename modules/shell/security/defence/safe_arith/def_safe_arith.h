#ifndef DEF_SAFE_ARITH_H
#define DEF_SAFE_ARITH_H

#include <stdint.h>

// Does 'value' fit a 32-bit int? 1 = yes, 0 = refused (logged).
int def_calc_fits(int64_t value);

// May 'a' be divided by 'b' (also for %)? 1 = yes, 0 = refused (logged).
int def_calc_divide_ok(int64_t a, int64_t b);

#endif
