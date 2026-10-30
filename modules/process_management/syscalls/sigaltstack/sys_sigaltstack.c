/*
 * sys_sigaltstack.c - sigaltstack(ss, old_ss).
 *
 * A handler set with SA_ONSTACK runs on this stack instead of the
 * program's own. That is how a program handles SIGSEGV from a stack
 * overflow: its own stack is full, the alternate one is not.
 *
 *   ss->flags = SS_DISABLE    no alternate stack any more
 *   otherwise                 [ss->sp, ss->sp + ss->size) becomes it:
 *                             at least MINSIGSTKSZ, mapped user memory
 * old_ss gets the previous one, flags SS_ONSTACK if a handler is
 * running on it now. While running on it, it cannot be changed.
 * 'user_esp' is the caller's stack pointer (to tell if it is on it).
 *
 * Uses: scheduler_get_current_process (core/scheduler), def_user_range_ok
 *       (memory_management defence user_ptr_bounds).
 */
#include "modules/process_management/syscalls/sigaltstack/sys_sigaltstack.h"                 // this file
#include "modules/process_management/core/scheduler/scheduler.h"                             // scheduler_get_current_process
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"  // def_user_range_ok

static int on_it(const process_t *p, uint32_t esp) {
    return p->altstack_size && esp >= p->altstack_sp && esp < p->altstack_sp + p->altstack_size;
}

int sys_sigaltstack(const k_stack_t *ss, k_stack_t *old_ss, uint32_t user_esp) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;

    k_stack_t old = { self->altstack_sp, self->altstack_flags, self->altstack_size };
    if (!self->altstack_size)
        old.flags = SS_DISABLE;
    else if (on_it(self, user_esp))
        old.flags = SS_ONSTACK;

    if (ss) {
        if (on_it(self, user_esp))
            return -1;                          // EPERM: a handler is using it
        if (ss->flags & SS_DISABLE) {
            self->altstack_sp = self->altstack_size = 0;
            self->altstack_flags = SS_DISABLE;
        } else {
            if (ss->size < MINSIGSTKSZ || !def_user_range_ok(ss->sp, ss->size))
                return -1;                      // ENOMEM / EFAULT
            self->altstack_sp = ss->sp;
            self->altstack_size = ss->size;
            self->altstack_flags = 0;
        }
    }
    if (old_ss)
        *old_ss = old;
    return 0;
}
