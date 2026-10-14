#include "proc_security.h"
#include "scheduler.h"
#include "process.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: stack overflow
 * ===================================================================
 * A process owns exactly PROCESS_STACK_SIZE (4 KB) of stack. Nothing
 * in the CPU stops deep recursion from pushing past the bottom into
 * whatever the heap allocated next: silent corruption of another
 * object, which is worse than a clean crash because nothing reports
 * it. launch() never overflows a real stack: it recurses SAFE_DEPTH
 * frames to measure one, then asks the guard about an ESP below it.
 * =================================================================== */

#define SAFE_DEPTH  8   /* fixed depth, far too small to reach the bottom */
#define FRAME_WORDS 8   /* real locals, so each call uses real stack      */

/* Bounded recursion used only to measure one frame. volatile keeps the
 * writes real, so this call chain cannot be optimised away. */
static void probe_frames(uint32_t depth, uint32_t *lowest) {
    volatile uint32_t pad[FRAME_WORDS];
    for (int i = 0; i < FRAME_WORDS; i++) pad[i] = depth + (uint32_t)i;
    if (depth == 0) {
        *lowest = (uint32_t)(uintptr_t)&pad[0];  /* deepest point we touched */
        return;
    }
    probe_frames(depth - 1, lowest);
}

/* simulate() only reads: no process is created, killed or changed. */
static int atk_stack_overflow_simulate(void) {
    process_t *me = scheduler_get_current_process();
    uint32_t room = (me && me->stack_base && me->esp >= me->stack_base) ? me->esp - me->stack_base : 0;
    vga_print_str("  stack per process : ");
    vga_print_int(PROCESS_STACK_SIZE);
    vga_print_str(" bytes\n  base / esp        : 0x");
    vga_print_hex(me ? me->stack_base : 0);
    vga_print_str(" / 0x");
    vga_print_hex(me ? me->esp : 0);
    vga_print_str("\n  bytes below esp   : ");
    vga_print_int((int)room);
    vga_print_str("\n  the CPU will not stop the next push past the base\n");
    return 1;
}

/* launch() demonstrates the detection, not the destruction. */
static int atk_stack_overflow_launch(void) {
    process_t *me = scheduler_get_current_process();
    uint32_t base = (me && me->stack_base) ? me->stack_base : 0x00200000u;
    uint32_t deepest = 0;
    process_t ghost;
    probe_frames(SAFE_DEPTH, &deepest);
    /* The address drop across the calls is roughly the frame cost. */
    uint32_t top = (uint32_t)(uintptr_t)&deepest;
    uint32_t frame = (top > deepest) ? (top - deepest) / SAFE_DEPTH : 0;
    vga_print_str("  one frame is about ");
    vga_print_int((int)frame);
    vga_print_str(" bytes, so this stack is about ");
    vga_print_int(frame ? (int)(PROCESS_STACK_SIZE / frame) : 0);
    vga_print_str(" frames deep\n");
    /* One push past the bottom is the whole attack. We only describe it. */
    ghost.stack_base = base;
    ghost.esp = base - 4;
    int verdict = def_stack_guard_check(&ghost);
    vga_print_str("  esp 0x");
    vga_print_hex(ghost.esp);
    vga_print_str(" vs base 0x");
    vga_print_hex(ghost.stack_base);
    vga_print_str(verdict ? " -> ALLOWED, isolation broken\n"
                          : " -> REFUSED, stack_guard fired\n");
    return verdict ? 0 : 1;
}

static const attack_t atk = {
    "stack_overflow",
    "recursion walks off the bottom of a 4 KB stack into the heap",
    INV_ISOLATION,
    SEV_CRITICAL,
    atk_stack_overflow_simulate,
    atk_stack_overflow_launch,
    0,                  /* nothing to undo: we never left our own stack */
    "stack_guard"
};

void atk_stack_overflow_register(void) {
    proc_security_register_attack(&atk);
}
