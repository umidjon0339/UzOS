/*
 * atk_kernel_patch.c - ATTACK kernel_patch: change one byte of kernel code.
 *
 * Breaks:     integrity - the kernel runs the code that was built
 * Stopped by: integrity_check (security/defence/integrity_check)
 *
 * Code in ring 0 can write anywhere. UzOS maps its own code writable,
 * and `exec` programs run in ring 0, so a malicious program could patch
 * the kernel: turn a check into "always yes", or hook a syscall. That is
 * what a rootkit does.
 *
 * The target here is harmless on purpose: atk_kernel_patch_target(), a
 * function nothing ever calls. We write `int3` (0xCC) over its first
 * byte. Nothing runs it, so nothing behaves differently: only a hash
 * of the code can tell.
 *
 * simulate(): show the target and measure the kernel (changes nothing).
 * launch():   1. patch the byte
 *             2. try to start /hello, as `exec` would (if it starts,
 *                stop it at once: /hello never ends by itself)
 *             3. put the byte back and measure again
 * cleanup():  put the byte back if it is still patched.
 *
 * Uses: integrity_verify (core/integrity), elf_load_and_run
 *       (process_management/core/elf), process_kill (core/process),
 *       ui_* (ui module).
 */

#include "registry.h"
#include "integrity.h"
#include "elf.h"
#include "process.h"
#include "ui.h"

#define PATCH_BYTE 0xCC         // int3: a breakpoint, if it ever ran

// The victim: kernel code that is never called. noinline + used keep the
// compiler from removing it; the empty asm keeps it a real function.
__attribute__((noinline, used)) void atk_kernel_patch_target(void) {
    asm volatile("");
}

static uint8_t saved;           // the original byte
static int     patched;         // 1 while our byte is in the kernel

static volatile uint8_t *target(void) {
    return (volatile uint8_t *)(uintptr_t)atk_kernel_patch_target;
}

static int restore(void) {
    if (!patched)
        return 0;
    *target() = saved;
    patched = 0;
    return 1;
}

static int simulate(void) {
    ui_printf("  target: atk_kernel_patch_target() at 0x%x, first byte %02x\n",
              (uint32_t)(uintptr_t)target(), *target());
    ui_printf("  writing %02x there would change nothing you can see:\n", PATCH_BYTE);
    ui_printf("  nothing calls it. Only a hash of .text can notice.\n");
    ui_printf("  kernel now: %s\n", integrity_verify() ? "unchanged since boot" : "CHANGED");
    return 1;
}

static int launch(void) {
    if (!integrity_verify()) {
        ui_printf("  the kernel is already changed: run `integrity`\n");
        return 0;
    }

    saved = *target();
    *target() = PATCH_BYTE;
    patched = 1;
    ui_printf("  patched 0x%x: %02x -> %02x\n",
              (uint32_t)(uintptr_t)target(), saved, PATCH_BYTE);

    // The real path: exec measures the kernel before it starts anything.
    ui_printf("  starting /hello on the patched kernel:\n");
    int pid = elf_load_and_run("/hello");
    int unnoticed = pid >= 0;
    ui_color(unnoticed ? UI_BAD : UI_GOOD);
    ui_printf("  -> %s\n", unnoticed ? "it ran: nobody noticed the patch"
                                     : "refused: the hash of .text changed");
    ui_reset();
    if (unnoticed)
        process_kill(pid);

    restore();
    ui_printf("  byte put back: kernel %s\n",
              integrity_verify() ? "unchanged again" : "STILL CHANGED");
    return unnoticed;
}

static int cleanup(void) {
    return restore();
}

static const attack_t attack = {
    "kernel_patch",
    "write one byte into the kernel's own code, then run a program",
    INV_INTEGRITY, SEV_CRITICAL,
    simulate, launch, cleanup,
    "integrity_check"
};

void atk_kernel_patch_register(void) {
    registry_add_attack(&attack);
}
