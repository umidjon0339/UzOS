#ifndef CALC_H
#define CALC_H

/*
 * calc.h - Evaluate an integer expression like "2 + 3 * (4 - 1)".
 */

#include <stdint.h>

// 1 and *result set if 'text' is a valid expression whose every step fits
// a 32-bit int. 0 and *error set to the reason otherwise.
int calc_eval(const char *text, int32_t *result, const char **error);

#endif
