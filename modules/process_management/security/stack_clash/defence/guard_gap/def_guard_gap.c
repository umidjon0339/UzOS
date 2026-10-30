/*
 * def_guard_gap.c - DEFENCE guard_gap: nothing lives right below a stack.
 *
 * Stops:  stack_clash (security/stack_clash)
 * Called: sys_execve() (syscalls/execve): the segments, before they are
 *         loaded; the gap, after the new stack is mapped.
 * Rules:  1. no segment of a program may lie in the execve stack area
 *            (0xB0000000..0xB0400000)
 *         2. the 8 pages (32 KiB) below a new stack must be unmapped
 * Why:    a stack grows down. If the page below it is mapped (a heap, a
 *         program segment), a deep recursion or a big local array
 *         walks straight into that memory and overwrites it - no fault,
 *         no warning: the "Stack Clash" (Qualys, 2017). With a gap of
 *         unmapped pages the CPU faults first. Linux keeps a 1 MiB
 *         stack_guard_gap for the same reason.
 *
 * Uses: vmm_walk (memory_management), EXEC_STACKS_* / PAGE_SIZE
 *       (memory layout), PT_LOAD (core/elf), audit_log, registry.
 */

#include "modules/process_management/security/stack_clash/defence/guard_gap/def_guard_gap.h"  // this file
#include "modules/memory_management/core/vmm/vmm.h"                  // vmm_walk
#include "modules/memory_management/core/layout/memory_layout.h"     // EXEC_STACKS_START, EXEC_STACKS_END
#include "modules/security_management/core/registry/registry.h"      // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"            // audit_log

// Does [start, start + size) touch [EXEC_STACKS_START, EXEC_STACKS_END)?
static int overlaps_stacks(uint32_t start, uint32_t size) {
    uint32_t end = start + size;
    return size && end > EXEC_STACKS_START && start < EXEC_STACKS_END;
}

int def_segments_clear_of_stacks(const Elf32_Phdr *ph, int count) {
    for (int i = 0; i < count; i++)
        if (ph[i].p_type == PT_LOAD && overlaps_stacks(ph[i].p_vaddr, ph[i].p_memsz)) {
            audit_log("guard_gap", "execve refused: a segment in the stack area");
            return 0;
        }
    return 1;
}

int def_guard_gap_ok(uint32_t stack_bottom) {
    for (uint32_t i = 1; i <= GUARD_PAGES; i++) {
        vmm_walk_t w;
        if (vmm_walk(stack_bottom - i * PAGE_SIZE, &w)) {
            audit_log("guard_gap", "execve refused: memory mapped right below the stack");
            return 0;
        }
    }
    return 1;
}

static int selftest(void) {
    if (!overlaps_stacks(EXEC_STACKS_START + 0x1000, 0x2000)) return 0;   // inside
    if (!overlaps_stacks(EXEC_STACKS_START - 0x1000, 0x2000)) return 0;   // reaches in
    if (overlaps_stacks(0x48000000, 0x1000)) return 0;                    // elsewhere
    if (overlaps_stacks(EXEC_STACKS_END, 0x1000)) return 0;               // just after
    if (overlaps_stacks(EXEC_STACKS_START, 0)) return 0;                  // empty
    return 1;
}

static const defence_t defence = {
    "guard_gap",
    "execve: no segment in the stack area; 32 KiB unmapped below a stack",
    "stack_clash",
    selftest
};

void def_guard_gap_register(void) { registry_add_defence(&defence); }
