#include "proc_security.h"

/* ===================================================================
 * DEFENCE: stack guard
 * ===================================================================
 * Restores INV_ISOLATION - a process must stay inside its own stack.
 *
 * Each process gets exactly PROCESS_STACK_SIZE bytes of stack. Nothing
 * in the CPU stops a deeply recursive function from walking straight
 * off the bottom of it and into whatever the heap allocated next. The
 * result is silent corruption of another object, which is far worse
 * than a crash.
 *
 * The check is placed in the scheduler, so it runs on every context
 * switch: a stack pointer outside its own range is caught before the
 * CPU is allowed to switch into it.
 *
 * The stronger fix is a guard page - unmap the page just below each
 * stack so the hardware faults immediately instead of writing. That
 * needs the per-process paging work first; this check is the cheap
 * half that works today.
 * =================================================================== */

/* pure policy: is this stack pointer inside the process's own stack? */
static int stack_pointer_ok(uint32_t esp, uint32_t base) {
    if (base == 0) return 1;                                 /* unguarded    */
    if (esp < base) return 0;                                /* past the end */
    if (esp >= base + PROCESS_STACK_SIZE) return 0;          /* outside all  */
    return 1;
}

int def_stack_guard_check(process_t *p) {
    if (!p) return 1;
    if (stack_pointer_ok(p->esp, p->stack_base)) return 1;

    sec_log("stack_guard", "stack pointer left its own stack");
    return 0;
}

static int def_stack_guard_selftest(void) {
    const uint32_t base = 0x00100000;   /* one page, 4096 bytes */

    /* just above the bottom: fine */
    if (stack_pointer_ok(base, base) != 1) return 0;

    /* near the top: fine */
    if (stack_pointer_ok(base + PROCESS_STACK_SIZE - 4, base) != 1) return 0;

    /* one byte below the bottom: the overflow we are looking for */
    if (stack_pointer_ok(base - 1, base) != 0) return 0;

    /* far below, e.g. walked into the heap */
    if (stack_pointer_ok(0x00000100, base) != 0) return 0;

    /* exactly at the top: outside */
    if (stack_pointer_ok(base + PROCESS_STACK_SIZE, base) != 0) return 0;

    /* an unguarded process is never rejected */
    if (stack_pointer_ok(0, 0) != 1) return 0;

    return 1;
}

static const defence_t def = {
    "stack_guard",
    "refuse to switch into a process whose stack pointer has escaped",
    "stack_overflow",
    def_stack_guard_selftest
};

void def_stack_guard_register(void) {
    proc_security_register_defence(&def);
}
