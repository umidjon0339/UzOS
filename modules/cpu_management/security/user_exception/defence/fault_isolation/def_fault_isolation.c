/*
 * def_fault_isolation.c - DEFENCE fault_isolation: a program's fault ends the program.
 *
 * Stops:  user_exception (security/attack/user_exception)
 * Called: every CPU exception handler: fault_handler() (core/faults) and
 *         the page fault handler (memory_management/core/vmm).
 * Rules:  1. the fault came from ring 3 (low 2 bits of the saved CS = 3)
 *         2. it is not one of the "machine is broken" exceptions:
 *            2 NMI, 8 double fault, 18 machine check
 *         Both true -> kill the process. Otherwise -> halt the kernel.
 * Why:    without a handler, every exception ended in "System Halted!".
 *         Then any user program could stop the whole OS with one
 *         `div` by zero or one `cli`. A ring 3 fault only proves that
 *         THIS program is broken; the kernel's own data is untouched.
 *         A ring 0 fault proves the kernel is broken: going on could
 *         corrupt everything, so stopping is the safe choice.
 */

#include "modules/cpu_management/security/user_exception/defence/fault_isolation/def_fault_isolation.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/security_management/core/audit/audit.h"

#define VECTOR_NMI           2
#define VECTOR_DOUBLE_FAULT  8
#define VECTOR_MACHINE_CHECK 18

// The whole rule, as a pure function so the selftest can try it.
static int survivable(uint32_t cs, uint32_t vector) {
    if ((cs & 3) != 3)
        return 0;                               // rule 1: kernel code faulted
    if (vector == VECTOR_NMI || vector == VECTOR_DOUBLE_FAULT ||
        vector == VECTOR_MACHINE_CHECK)
        return 0;                               // rule 2: hardware trouble
    return vector < ISR_EXCEPTIONS;
}

int def_fault_isolation_survivable(const registers_t *regs) {
    if (!survivable(regs->cs, regs->int_no))
        return 0;
    audit_log("fault_isolation", "ring 3 exception: killed the process, not the OS");
    return 1;
}

static int selftest(void) {
    const uint32_t user_cs = 0x1B, kernel_cs = 0x08;    // see arch/x86/gdt

    if (!survivable(user_cs, 0)) return 0;              // ring 3 divide error
    if (!survivable(user_cs, 6)) return 0;              // ring 3 invalid opcode
    if (!survivable(user_cs, 13)) return 0;             // ring 3 cli / in / hlt
    if (!survivable(user_cs, 14)) return 0;             // ring 3 page fault
    if (survivable(kernel_cs, 0)) return 0;             // kernel bug: halt
    if (survivable(kernel_cs, 14)) return 0;
    if (survivable(user_cs, VECTOR_DOUBLE_FAULT)) return 0;
    if (survivable(user_cs, VECTOR_MACHINE_CHECK)) return 0;
    if (survivable(user_cs, 128)) return 0;             // not an exception
    return 1;
}

static const defence_t defence = {
    "fault_isolation",
    "a CPU exception in ring 3 kills that process, not the whole OS",
    "user_exception",
    selftest
};

void def_fault_isolation_register(void) {
    registry_add_defence(&defence);
}
