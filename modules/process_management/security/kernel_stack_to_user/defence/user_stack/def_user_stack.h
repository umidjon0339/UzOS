#ifndef DEF_USER_STACK_H
#define DEF_USER_STACK_H

#include <stdint.h>

// Is 'esp' a safe stack pointer to give to ring 3? 1 = yes, 0 = refused (logged).
int def_user_stack_ok(uint32_t esp);

#endif
