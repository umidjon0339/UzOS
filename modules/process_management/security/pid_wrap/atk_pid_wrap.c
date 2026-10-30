/*
 * atk_pid_wrap.c - ATTACK pid_wrap: make two processes share a PID.
 *
 * Breaks:     identity - a PID must name exactly one process
 * Stopped by: pid_bounds (defence/pid_bounds)
 *
 * PIDs come from a counter that only goes up. After 0xFFFFFFFF a 32-bit
 * counter wraps to 0, then 1, 2, ... and those PIDs may still be alive.
 *
 * simulate(): show the highest live PID and which live PIDs a wrap would hit.
 * launch():   4 billion creations is too many to run, so ask the guard
 *             about the PIDs a wrap would produce.
 *
 * Uses: def_pid_bounds_check (defence/pid_bounds), the process table.
 */

#include "modules/security_management/core/registry/registry.h"                              // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"                                 // process_t, process_get_table
#include "modules/device_management/vga/vga.h"                                               // vga_print_str, vga_print_int, vga_print_hex
#include "modules/process_management/security/pid_wrap/defence/pid_bounds/def_pid_bounds.h"  // PID_LIMIT, def_pid_bounds_check

#define SMALL_PID 32u           // a wrapped counter hits these first

static int simulate(void) {
    process_t *table = process_get_table();
    uint32_t highest = 0;
    int collisions = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (table[i].state == PROCESS_STATE_FREE)
            continue;
        if (table[i].pid > highest)
            highest = table[i].pid;
        if (table[i].pid < SMALL_PID)
            collisions++;
    }

    vga_print_str("  highest live PID           : ");
    vga_print_int(highest);
    vga_print_str("\n  live PIDs a wrap would reuse: ");
    vga_print_int(collisions);
    vga_print_str("\n");
    return collisions;
}

static int launch(void) {
    static const uint32_t probes[] = {
        1,                  // normal PID: allowed
        PID_LIMIT - 1,      // last PID the guard hands out
        PID_LIMIT,          // first PID the guard refuses
        0xFFFFFFFF,         // the counter wraps right after this
        0,                  // what a wrap produces first: "no process"
    };
    int refused = 0;

    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
        int ok = def_pid_bounds_check(probes[i]);
        refused += !ok;

        vga_print_str("  PID ");
        vga_print_hex(probes[i]);
        vga_print_str(ok ? "  accepted\n" : "  refused by pid_bounds\n");
    }
    return refused;
}

static const attack_t attack = {
    "pid_wrap",
    "the PID counter wraps around and reuses a live PID",
    INV_IDENTITY, SEV_HIGH,
    simulate, launch, 0,        // nothing created, nothing to undo
    "pid_bounds"
};

void atk_pid_wrap_register(void) {
    registry_add_attack(&attack);
}
