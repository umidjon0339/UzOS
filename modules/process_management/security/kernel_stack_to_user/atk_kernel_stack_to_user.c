/*
 * atk_kernel_stack_to_user.c - ATTACK kernel_stack_to_user: give ring 3 a
 * kernel stack.
 *
 * Breaks:     privilege - user code must not write kernel memory
 * Stopped by: user_stack (defence/user_stack)
 *
 * Entering ring 3 means handing the CPU a stack pointer. User code then
 * writes to that stack freely. If it is a kernel stack, user code can
 * overwrite saved kernel registers and return addresses. The first
 * `testuser` did exactly this: it passed its own kernel ESP.
 *
 * simulate(): show the kernel addresses that would be dangerous.
 * launch():   offer each of them to the guard as a "user stack". It never
 *             really enters ring 3 with them.
 *
 * Uses: def_user_stack_ok (defence/user_stack), cpu_read_esp
 *       (arch/x86/cpu), stack_top (boot/boot.s).
 */

#include "modules/security_management/core/registry/registry.h"                                          // attack_t, registry_add_attack
#include "arch/x86/cpu/cpu.h"                                                                            // cpu_read_esp
#include "modules/memory_management/core/layout/memory_layout.h"                                         // KHEAP_START
#include "modules/device_management/vga/vga.h"                                                           // vga_print_str, vga_print_hex
#include "modules/process_management/security/kernel_stack_to_user/defence/user_stack/def_user_stack.h"  // def_user_stack_ok

extern uint8_t stack_top[];     // top of the boot stack (boot/boot.s)

typedef struct {
    const char *what;
    uint32_t    esp;
} candidate_t;

// Fill 'out' with kernel stack pointers an attacker could pass. Returns count.
static int collect(candidate_t out[3]) {
    out[0] = (candidate_t){ "our own kernel ESP   ", cpu_read_esp() };
    out[1] = (candidate_t){ "top of the boot stack", (uint32_t)stack_top };
    out[2] = (candidate_t){ "the kernel heap      ", KHEAP_START };
    return 3;
}

static void print_candidate(const candidate_t *c) {
    vga_print_str("  ");
    vga_print_str(c->what);
    vga_print_str(" ");
    vga_print_hex(c->esp);
}

static int simulate(void) {
    candidate_t c[3];
    int n = collect(c);

    for (int i = 0; i < n; i++) {
        print_candidate(&c[i]);
        vga_print_str("\n");
    }
    vga_print_str("  all are kernel memory; user space starts at 0x08000000\n");
    return n;
}

static int launch(void) {
    candidate_t c[3];
    int n = collect(c);
    int refused = 0;

    for (int i = 0; i < n; i++) {
        int ok = def_user_stack_ok(c[i].esp);
        refused += !ok;
        print_candidate(&c[i]);
        vga_print_str(ok ? "  -> ACCEPTED for ring 3\n" : "  -> refused\n");
    }
    return refused;
}

static const attack_t attack = {
    "kernel_stack_to_user",
    "enter ring 3 with a stack pointer into kernel memory",
    INV_PRIVILEGE, SEV_CRITICAL,
    simulate, launch, 0,        // never entered ring 3: nothing to undo
    "user_stack"
};

void atk_kernel_stack_to_user_register(void) {
    registry_add_attack(&attack);
}
