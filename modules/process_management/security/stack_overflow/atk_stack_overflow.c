/*
 * atk_stack_overflow.c - ATTACK stack_overflow: recurse off the stack.
 *
 * Breaks:     isolation - a process must stay inside its own 4 KiB stack
 * Stopped by: stack_guard (defence/stack_guard)
 *
 * Each process has PROCESS_STACK_SIZE bytes of stack. Deep recursion
 * keeps pushing below the bottom and the CPU does not stop it: it
 * silently overwrites whatever the heap placed there.
 *
 * simulate(): show the current stack and how much room is left.
 * launch():   measure one stack frame with a short, safe recursion, then
 *             ask the guard about an ESP one push below a stack's bottom.
 *             (It never really overflows: that would corrupt the heap.)
 *
 * Uses: def_stack_guard_check (defence/stack_guard),
 *       scheduler_get_current_process (core/scheduler), cpu_read_esp (arch/x86/cpu).
 */

#include "modules/security_management/core/registry/registry.h"                                      // attack_t, registry_add_attack
#include "modules/process_management/core/scheduler/scheduler.h"                                     // scheduler_get_current_process
#include "modules/process_management/core/process/process.h"                                         // process_t, PROCESS_STACK_SIZE
#include "modules/device_management/vga/vga.h"                                                       // vga_print_str, vga_print_int, vga_print_hex
#include "arch/x86/cpu/cpu.h"                                                                        // volatile
#include "modules/process_management/security/stack_overflow/defence/stack_guard/def_stack_guard.h"  // def_stack_guard_check

#define SAFE_DEPTH  8           // far too shallow to reach the bottom
#define FRAME_WORDS 8           // real local data in every frame
#define FAKE_BASE   0x00200000  // stand-in stack for kernel_main (boot stack)

// Recurse 'depth' times; report the lowest stack address touched.
// 'volatile' keeps the compiler from optimising the frames away.
static void probe_frames(uint32_t depth, uint32_t *lowest) {
    volatile uint32_t pad[FRAME_WORDS];
    for (int i = 0; i < FRAME_WORDS; i++)
        pad[i] = depth;

    if (depth == 0)
        *lowest = (uint32_t)&pad[0];
    else
        probe_frames(depth - 1, lowest);
}

static int simulate(void) {
    process_t *me = scheduler_get_current_process();
    uint32_t esp = cpu_read_esp();

    vga_print_str("  stack size per process : ");
    vga_print_int(PROCESS_STACK_SIZE);
    vga_print_str(" bytes\n  this process           : ");
    vga_print_str(me->name);
    if (!me->stack_base) {
        vga_print_str(" (boot stack, not guarded)\n");
        return 1;
    }
    vga_print_str("\n  bytes left below ESP   : ");
    vga_print_int(esp - me->stack_base);
    vga_print_str("\n  nothing stops the next push past the bottom\n");
    return 1;
}

static int launch(void) {
    uint32_t top = cpu_read_esp();
    uint32_t lowest = top;
    probe_frames(SAFE_DEPTH, &lowest);

    uint32_t frame = (top - lowest) / SAFE_DEPTH;
    vga_print_str("  one frame ~ ");
    vga_print_int(frame);
    vga_print_str(" bytes -> a 4 KiB stack holds ~");
    vga_print_int(frame ? PROCESS_STACK_SIZE / frame : 0);
    vga_print_str(" frames\n");

    // One push past the bottom is the whole attack. Ask the guard.
    process_t *me = scheduler_get_current_process();
    process_t victim = {0};
    victim.stack_base = me->stack_base ? me->stack_base : FAKE_BASE;
    uint32_t bad_esp = victim.stack_base - 4;

    int allowed = def_stack_guard_check(&victim, bad_esp);
    vga_print_str("  ESP ");
    vga_print_hex(bad_esp);
    vga_print_str(allowed ? " -> ALLOWED, overflow missed\n"
                          : " -> caught by stack_guard\n");
    return !allowed;
}

static const attack_t attack = {
    "stack_overflow",
    "recursion runs off the bottom of a 4 KiB stack into the heap",
    INV_ISOLATION, SEV_CRITICAL,
    simulate, launch, 0,        // nothing to undo: no real stack was overflowed
    "stack_guard"
};

void atk_stack_overflow_register(void) {
    registry_add_attack(&attack);
}
