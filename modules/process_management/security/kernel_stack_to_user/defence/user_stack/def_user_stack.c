/*
 * def_user_stack.c - DEFENCE user_stack: ring 3 only gets a user stack.
 *
 * Stops:  kernel_stack_to_user (security/kernel_stack_to_user)
 * Called: `testuser` (commands/testuser), right before switch_to_user_mode().
 * Rules:  1. USER_SPACE_START < esp <= USER_SPACE_END
 *         2. esp is 4-byte aligned
 * Why:    user code writes freely to its own stack. If that "stack" were
 *         a kernel stack, the heap, or the kernel image, ring 3 could
 *         overwrite kernel data and return addresses and take over.
 *         (The top, USER_SPACE_END itself, is allowed: the first push
 *         goes below it.)
 */

#include "modules/process_management/security/kernel_stack_to_user/defence/user_stack/def_user_stack.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                          // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                // audit_log
#include "modules/memory_management/core/layout/memory_layout.h"                                         // USER_SPACE_START, USER_SPACE_END

// The whole rule, as a pure function so the selftest can try it.
static int user_stack_ok(uint32_t esp) {
    if (esp <= USER_SPACE_START || esp > USER_SPACE_END) return 0;   // rule 1
    return (esp & 3) == 0;                                           // rule 2
}

int def_user_stack_ok(uint32_t esp) {
    if (user_stack_ok(esp))
        return 1;

    audit_log("user_stack", "refused to give ring 3 a stack outside user space");
    return 0;
}

static int selftest(void) {
    if (user_stack_ok(0x40002000) != 1) return 0;           // a real user stack
    if (user_stack_ok(USER_SPACE_END) != 1) return 0;       // very top
    if (user_stack_ok(0) != 0) return 0;
    if (user_stack_ok(0x00119000) != 0) return 0;           // boot stack area
    if (user_stack_ok(0x01000000) != 0) return 0;           // kernel heap
    if (user_stack_ok(USER_SPACE_START) != 0) return 0;     // nothing below to push into
    if (user_stack_ok(USER_SPACE_END + 4) != 0) return 0;   // above user space
    if (user_stack_ok(0x40001FFE) != 0) return 0;           // misaligned
    return 1;
}

static const defence_t defence = {
    "user_stack",
    "only hand ring 3 a stack that lies in user space",
    "kernel_stack_to_user",
    selftest
};

void def_user_stack_register(void) {
    registry_add_defence(&defence);
}
