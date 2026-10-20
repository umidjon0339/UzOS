#ifndef DEF_SLOT_QUOTA_H
#define DEF_SLOT_QUOTA_H

#include <stdint.h>

// May user 'uid' create one more process? 1 = yes, 0 = refused (logged).
int def_slot_quota_check(uint32_t uid);

#endif
