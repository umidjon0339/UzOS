#ifndef DEF_NAME_GUARD_H
#define DEF_NAME_GUARD_H

#include "modules/process_management/core/process/process.h"   // process_t

// May 'p' call itself 'name' (prctl PR_SET_NAME)? The name must be
// printable text (no control characters or escape sequences), not
// empty, and not the name of a system process. 1 = allowed, 0 = refused
// (logged).
int def_name_ok(const process_t *p, const char *name);

#endif
