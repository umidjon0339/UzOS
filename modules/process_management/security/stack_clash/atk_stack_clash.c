/*
 * atk_stack_clash.c - ATTACK stack_clash: put memory right under a stack.
 *
 * Breaks:     isolation - a stack must fault before it overwrites anything
 * Stopped by: guard_gap (defence/guard_gap)
 *
 * A stack grows down silently. If something is mapped just below it, a
 * deep recursion or a large local array writes into that memory with
 * no fault at all. Two ways to arrange it: an ELF program whose segment
 * lies in the stack area, or a mapping placed right under the stack.
 *
 * simulate(): explain.
 * launch():   1. a program header with its segment inside the execve
 *                stack area, checked as execve checks it
 *             2. map one page right below the stack of the last process
 *                slot, check the gap as execve does, unmap it again
 *             findings = arrangements accepted.
 * cleanup():  nothing (launch unmaps its page).
 *
 * Uses: def_segments_clear_of_stacks / def_guard_gap_ok (its defence),
 *       pmm_alloc_block / pmm_free_block, vmm_map_page / vmm_unmap_page
 *       (memory_management), EXEC_STACKS_START (memory layout), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/stack_clash/defence/guard_gap/def_guard_gap.h"  // def_segments_clear_of_stacks, def_guard_gap_ok
#include "modules/process_management/syscalls/execve/sys_execve.h"     // EXEC_STACK_PAGES
#include "modules/process_management/core/process/process.h"           // MAX_PROCESSES
#include "modules/memory_management/core/pmm/pmm.h"                    // pmm_alloc_block, pmm_free_block
#include "modules/memory_management/core/vmm/vmm.h"                    // vmm_map_page, vmm_unmap_page
#include "modules/memory_management/core/layout/memory_layout.h"       // EXEC_STACKS_START
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_color

#define AREA_SIZE 0x10000

static int simulate(void) {
    ui_printf("  1. an ELF segment inside the stack area\n");
    ui_printf("  2. a page mapped right below a stack\n");
    ui_printf("  either way the stack grows into it without a fault\n");
    return 2;
}

static int verdict(const char *what, int accepted) {
    ui_printf("  %-40s ", what);
    ui_color(accepted ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", accepted ? "accepted: no guard!" : "refused");
    ui_reset();
    return accepted;
}

static int launch(void) {
    int bad = 0;
    Elf32_Phdr ph = { PT_LOAD, 0, EXEC_STACKS_START + 0x8000, EXEC_STACKS_START + 0x8000,
                      0x1000, 0x2000, 5, 0x1000 };
    bad += verdict("segment at 0xB0008000 (stack area)", def_segments_clear_of_stacks(&ph, 1));

    // The last slot's stack: its bottom, and the page right below it.
    uint32_t top    = EXEC_STACKS_START + MAX_PROCESSES * AREA_SIZE;
    uint32_t bottom = top - EXEC_STACK_PAGES * PAGE_SIZE;
    void *frame = pmm_alloc_block();
    if (frame && vmm_map_page(bottom - PAGE_SIZE, (uint32_t)frame, PAGE_WRITABLE | PAGE_USER)) {
        bad += verdict("a page mapped right below a stack", def_guard_gap_ok(bottom));
        vmm_unmap_page(bottom - PAGE_SIZE);
    }
    if (frame)
        pmm_free_block(frame);
    return bad;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "stack_clash",
    "a segment or a mapping right under a stack: it grows into it silently",
    INV_ISOLATION, SEV_MEDIUM,
    simulate, launch, cleanup,
    "guard_gap"
};

void atk_stack_clash_register(void) { registry_add_attack(&attack); }
