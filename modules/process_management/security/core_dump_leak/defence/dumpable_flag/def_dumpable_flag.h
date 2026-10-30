#ifndef DEF_DUMPABLE_FLAG_H
#define DEF_DUMPABLE_FLAG_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// 'p' changes its uid from 'old_uid' to 'new_uid' (setuid). If it really
// changes, 'p' becomes NOT dumpable: what it held under the old user
// (keys read as root) must not be readable by the new one. setuid()
// (user_management) calls this. Returns p's dumpable flag afterwards.
int def_dumpable_on_setuid(process_t *p, uint32_t old_uid, uint32_t new_uid);

#endif
