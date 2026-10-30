/*
 * def_stack_guard.c - DEFENCE stack_guard: catch a stack that overflowed.
 *
 * Stops:  stack_overflow (security/stack_overflow)
 * Called: scheduler_yield() (core/scheduler), on every task switch, with
 *         the real ESP of the running process.
 * Rule:   stack_base <= esp <= stack_base + PROCESS_STACK_SIZE
 * Why:    the CPU does not stop a push below the bottom of a 4 KiB stack;
 *         it silently overwrites the heap. This check notices it at the
 *         next switch and retires the process.
 *         A stronger fix is a "guard page": an unmapped page below each
 *         stack, so the very first bad push faults. That needs per-process
 *         paging first.
 */

#include "modules/process_management/security/stack_overflow/defence/stack_guard/def_stack_guard.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                      // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                            // audit_log

// The whole rule, as a pure function so the selftest can try it.
static int stack_pointer_ok(uint32_t esp, uint32_t base) {
    if (base == 0) return 1;                            // boot stack: not ours to check
    if (esp < base) return 0;                           // ran off the bottom
    if (esp > base + PROCESS_STACK_SIZE) return 0;      // not this stack at all
    return 1;
}

int def_stack_guard_check(const process_t *p, uint32_t esp) {
    if (!p || stack_pointer_ok(esp, p->stack_base))
        return 1;

    audit_log("stack_guard", "stack pointer left its own stack");
    return 0;
}

static int selftest(void) {
    const uint32_t base = 0x00200000;
    const uint32_t top  = base + PROCESS_STACK_SIZE;

    if (stack_pointer_ok(top - 4, base) != 1) return 0;     // normal use
    if (stack_pointer_ok(base, base) != 1) return 0;        // completely full
    if (stack_pointer_ok(base - 4, base) != 0) return 0;    // one push too many
    if (stack_pointer_ok(0x100, base) != 0) return 0;       // far below
    if (stack_pointer_ok(top + 4, base) != 0) return 0;     // above the stack
    if (stack_pointer_ok(0x1234, 0) != 1) return 0;         // unguarded boot stack
    return 1;
}

static const defence_t defence = {
    "stack_guard",
    "retire a process whose stack pointer left its own stack",
    "stack_overflow",
    selftest
};

void def_stack_guard_register(void) {
    registry_add_defence(&defence);
}
