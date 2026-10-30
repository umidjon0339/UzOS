#ifndef DEF_OPTLEN_CHECK_H
#define DEF_OPTLEN_CHECK_H

/*
 * def_optlen_check.h - DEFENCE optlen_check: an option's length must be at least the size of the option
 */

#include <stdint.h>

// Is 'len' enough for option 'name' of 'level'? socket_setopt() asks.
// 1 = yes, 0 = EINVAL (logged).
int def_optlen_ok(int level, int name, uint32_t len);

#endif
