/*
 * usertask.c - Copy a program to the user pages and drop it to ring 3.
 *
 * A new process starts in ring 0 like every other one. Its first code
 * (user_entry below) maps the two user pages, copies the program in,
 * and then calls switch_to_user_mode(): from there on it is a ring 3
 * process that can only reach the kernel through int 0x80 (or by
 * causing a CPU exception, which kills it).
 *
 * Uses: process_create / process_get_by_pid (core/process), pmm_alloc_block
 *       + vmm_map_page (memory_management), switch_to_user_mode
 *       (arch/x86/usermode), def_user_stack_ok (security/defence/user_stack),
 *       sys_sleep (syscalls/sleep), ui_error (ui).
 */

#include "modules/process_management/core/usertask/usertask.h"                                           // this file
#include "modules/process_management/core/process/process.h"                                             // process_create, process_t
#include "modules/memory_management/core/pmm/pmm.h"                                                      // pmm_alloc_block
#include "modules/memory_management/core/vmm/vmm.h"                                                      // vmm_map_page, PAGE_WRITABLE, PAGE_USER
#include "lib/string/string.h"                                                                           // memcpy, memset
#include "arch/x86/usermode/usermode.h"                                                                  // switch_to_user_mode
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                                         // sys_sleep
#include "modules/ui/core/style/ui.h"                                                                    // ui_error
#include "modules/process_management/security/kernel_stack_to_user/defence/user_stack/def_user_stack.h"  // def_user_stack_ok

#define WAIT_STEP_MS 10

// The program the next user task copies. One at a time (see usertask.h).
static const uint8_t *program_start;
static const uint8_t *program_end;

// Map the two user pages once; later runs reuse them.
static int map_user_pages(void) {
    static int mapped;
    const uint32_t pages[] = { USERTASK_CODE, USERTASK_STACK };

    if (mapped)
        return 1;

    for (int i = 0; i < 2; i++) {
        void *frame = pmm_alloc_block();
        if (!frame || !vmm_map_page(pages[i], (uint32_t)frame, PAGE_WRITABLE | PAGE_USER))
            return 0;
    }
    mapped = 1;
    return 1;
}

// Runs as the new process, still in ring 0: prepare, then enter ring 3.
static void user_entry(void) {
    if (!map_user_pages()) {
        ui_error("usertask", "out of memory");
        return;
    }

    memcpy((void *)USERTASK_CODE, program_start, program_end - program_start);
    memset((void *)USERTASK_MAILBOX, 0, USERTASK_MAILBOX_SIZE);
    uint32_t user_esp = USERTASK_STACK + PAGE_SIZE;     // stack grows down from the top

    // DEFENCE user_stack: ring 3 must only ever get a user-space stack.
    if (!def_user_stack_ok(user_esp)) {
        ui_error("usertask", "refused, not a user-space stack");
        return;
    }

    switch_to_user_mode(USERTASK_CODE, user_esp);       // never returns
}

int usertask_start(const char *name, const uint8_t *start, const uint8_t *end,
                   uint32_t uid) {
    if (end <= start || end - start > USERTASK_MAX_CODE)
        return -1;                                      // would overwrite the mailbox

    program_start = start;
    program_end   = end;
    return process_create(name, user_entry, uid);
}

static int has_ended(int pid) {
    process_t *p = process_get_by_pid(pid);
    return !p || p->state == PROCESS_STATE_ZOMBIE;
}

int usertask_wait(int pid, uint32_t timeout_ms) {
    for (uint32_t waited = 0; waited < timeout_ms; waited += WAIT_STEP_MS) {
        if (has_ended(pid))
            return 1;
        sys_sleep(WAIT_STEP_MS);
    }
    return has_ended(pid);
}

volatile uint32_t *usertask_mailbox(void) {
    return (volatile uint32_t *)USERTASK_MAILBOX;
}
