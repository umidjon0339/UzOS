/*
 * cmd_pmap.c - `pmap [pid]`: the memory map of a process.
 *
 *   5:   worker
 *   00123000      4K rw-  [kernel stack]     (0 for normal users: kptr_hide)
 *   40000000      8K rw-  [usertask code+stack]
 *   50000000     12K rw-  [heap: sbrk]
 *    total        24K
 *
 * The user part comes from the page tables: runs of mapped user pages,
 * walked 4 MiB at a time (a missing page table skips 1024 pages at once).
 * UzOS has ONE address space, so every process sees the same user pages;
 * only the kernel stack is its own. That is why fork() and real isolation
 * wait for per-process page directories. The map is the owner's
 * business (hidepid).
 *
 * Uses: process_get_by_pid / process_caller_uid (core/process),
 *       scheduler_get_current_process, vmm_walk / PAGE_* (memory_management),
 *       USER_* (memory layout), def_hidepid_visible, def_kptr_show (security/),
 *       atoi, ui.
 */

#include "modules/process_management/commands/pmap/cmd_pmap.h"         // this file
#include "modules/process_management/core/process/process.h"           // process_get_by_pid, process_caller_uid
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/memory_management/core/vmm/vmm.h"                    // vmm_walk, PAGE_USER, PAGE_WRITABLE
#include "modules/memory_management/core/layout/memory_layout.h"       // USER_SPACE_START, USER_BRK_START, ...
#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"          // def_hidepid_visible
#include "modules/process_management/security/kernel_pointer_leak/defence/kptr_hide/def_kptr_hide.h"  // def_kptr_show
#include "lib/stdlib/stdlib.h"                                         // atoi
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_error

#define TABLE_SPAN (PAGE_SIZE * PAGES_PER_TABLE)    // 4 MiB: one page table

static const char *region(uint32_t addr) {
    if (addr >= USER_TEST_CODE && addr < USER_TEST_CODE + 2 * PAGE_SIZE) return "[usertask code+stack]";
    if (addr >= USER_BRK_START && addr < USER_BRK_END)                  return "[heap: sbrk]";
    if (addr >= MMAP_START && addr < MMAP_END)                          return "[mmap]";
    return "[user]";
}

static void line(uint32_t start, uint32_t bytes, int writable, const char *what) {
    ui_printf("%08x %8uK r%c-  %s\n", start, bytes / 1024, writable ? 'w' : '-', what);
}

void cmd_pmap(const char *args) {
    process_t *p = args[0] ? process_get_by_pid((uint32_t)atoi(args)) : scheduler_get_current_process();
    uint32_t viewer = process_caller_uid();
    if (!p) {
        ui_error("pmap", "no such process (usage: pmap [pid])");
        return;
    }
    ui_printf("%u:   %s\n", p->pid, p->name);
    if (!def_hidepid_visible(viewer, p)) {      // DEFENCE hidepid
        ui_printf("(hidden: not your process)\n");
        return;
    }

    uint32_t total = 0;
    if (p->stack_base) {                        // DEFENCE kptr_hide on the address
        line(def_kptr_show(viewer, p->stack_base), PROCESS_STACK_SIZE, 1, "[kernel stack]");
        total += PROCESS_STACK_SIZE;
    }

    uint32_t run_start = 0, run_bytes = 0;
    int run_writable = 0;
    for (uint32_t addr = USER_SPACE_START; addr < USER_SPACE_END; ) {
        vmm_walk_t w;
        int mapped = vmm_walk(addr, &w) && (w.pte & PAGE_USER);
        int writable = (w.pte & PAGE_WRITABLE) != 0;
        if (run_bytes && (!mapped || writable != run_writable ||
                          region(addr) != region(run_start))) {
            line(run_start, run_bytes, run_writable, region(run_start));
            total += run_bytes;
            run_bytes = 0;
        }
        if (mapped) {
            if (!run_bytes) {
                run_start = addr;
                run_writable = writable;
            }
            run_bytes += PAGE_SIZE;
        }
        addr += !w.pde ? TABLE_SPAN - (addr % TABLE_SPAN) : PAGE_SIZE;  // no table: skip it
    }
    if (run_bytes) {
        line(run_start, run_bytes, run_writable, region(run_start));
        total += run_bytes;
    }
    ui_printf(" total %8uK   (the user pages are shared: UzOS has one address space)\n",
              total / 1024);
}
