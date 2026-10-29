/*
 * atk_uninit_pcb.c - ATTACK uninit_pcb: read the last owner's data.
 *
 * Breaks:     disclosure - a new process must not see another's data
 * Stopped by: pcb_wipe (defence/pcb_wipe)
 *
 * Slots are reused. If process_create() only writes some fields, the
 * others still hold the old owner's values (open files, cwd, ...).
 *
 * simulate(): count free slots that still hold old data.
 * launch():   leave fake old data in the next free slot, create a process
 *             there, and count what the new process inherited.
 * cleanup():  kill the probe process.
 *
 * Uses: process_create / process_kill / the process table (core/process).
 */

#include "registry.h"
#include "process.h"
#include "vga.h"

#define FAKE_POINTER ((struct vfs_node *)0xDEADBEEF)

static uint32_t probe_pid;      // the process launch() created

static void probe_worker(void) {
    while (1)
        asm volatile("hlt");
}

// Fields process_create() never sets. Non-zero on a new process = leaked.
static int leaked_fields(const process_t *p) {
    int n = (p->cwd != 0) + (p->sleep_ticks != 0) + (p->is_system != 0);
    for (int i = 0; i < MAX_FDS; i++)
        n += (p->fd_table[i] != 0) + (p->fd_offset[i] != 0) + (p->fd_flags[i] != 0);
    return n;
}

static process_t *first_free_slot(void) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state == PROCESS_STATE_FREE)
            return &table[i];
    return 0;
}

static int simulate(void) {
    process_t *table = process_get_table();
    int dirty = 0;

    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state == PROCESS_STATE_FREE && leaked_fields(&table[i]))
            dirty++;

    vga_print_str("  free slots holding old data: ");
    vga_print_int(dirty);
    vga_print_str("\n");
    return dirty;
}

static int launch(void) {
    process_t *slot = first_free_slot();     // process_create() picks this one
    if (!slot)
        return 0;

    // Play the previous owner: leave a cwd and an open file behind.
    slot->cwd = FAKE_POINTER;
    slot->fd_table[3] = FAKE_POINTER;
    slot->fd_offset[3] = 512;

    int pid = process_create("uninit_probe", probe_worker, 0);
    if (pid < 0)
        return 0;
    probe_pid = pid;

    int leaked = leaked_fields(slot);       // same slot, new owner
    vga_print_str("  fields inherited from the old owner: ");
    vga_print_int(leaked);
    vga_print_str(leaked ? "  LEAK\n" : "  (slot was wiped first)\n");
    return leaked;
}

static int cleanup(void) {
    int killed = probe_pid && process_kill(probe_pid);
    probe_pid = 0;
    return killed;
}

static const attack_t attack = {
    "uninit_pcb",
    "a reused process slot hands its old owner's data to the new process",
    INV_DISCLOSURE, SEV_HIGH,
    simulate, launch, cleanup,
    "pcb_wipe"
};

void atk_uninit_pcb_register(void) {
    registry_add_attack(&attack);
}
