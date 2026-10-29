/*
 * atk_process_injection.c - ATTACK process_injection (concept).
 *
 * Breaks:     isolation - one process must not write another's memory
 * Stopped by: process_isolation (defence/process_isolation)
 *
 * Injection writes code or data into a running process's memory (think
 * ptrace POKETEXT, or /proc/<pid>/mem) and then makes it run that code.
 * The barrier is a separate address space per process, so one cannot
 * reach another's memory at all. UzOS isolates ring 3 from the kernel
 * but its kernel-mode tasks share one address space, so a full demo is
 * a concept here; the lesson is what real isolation requires.
 *
 * Uses: process_isolation (its defence), vga.
 */

#include "registry.h"
#include "def_process_isolation.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  write code into another process, then make it run\n");
    vga_print_str("  barrier: a separate address space per process\n");
    vga_print_str("  UzOS walls off ring 3 from the kernel (PAGE_USER)\n");
    return 1;
}
static int launch(void) {
    vga_print_str("  not run: a safe demo needs per-process address spaces\n");
    return def_ring3_isolated() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "process_injection",
    "write code into another process's memory and run it (concept)",
    INV_ISOLATION, SEV_INFO,
    simulate, launch, cleanup,
    "process_isolation"
};
void atk_process_injection_register(void) { registry_add_attack(&attack); }
