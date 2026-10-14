#include "proc_security.h"
#include "process.h"
#include "scheduler.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: uninitialised PCB - a recycled slot leaks its last owner.
 * Break INV_DISCLOSURE: a new process must never see another's data.
 * A partly written slot keeps the old cwd, file descriptors and
 * offsets. We recycle a real slot and count what the new process
 * inherited; zero inherited fields means it was cleaned first.
 * =================================================================== */
#define PROBE_NAME "uninit_probe"
static uint32_t probe_pid;   /* the one process launch() created */
static void probe_worker(void) { while (1) asm volatile("hlt"); }
/* process_create() never writes these fields, so a non-zero value on
 * a fresh process was inherited from the previous owner of the slot. */
static int sticky_fields(const process_t *p) {
    int n = (p->cwd != 0) + (p->parent_pid != 0) +
            (p->child_count != 0) + (p->sleep_ticks != 0);
    for (int i = 0; i < MAX_FDS; i++)
        n += (p->fd_table[i] != 0) + (p->fd_offset[i] != 0);
    return n;
}

/* simulate() reads the table and changes nothing at all. */
static int atk_uninit_pcb_simulate(void) {
    process_t *t = process_get_table();
    int dirty = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (t[i].state != PROCESS_STATE_FREE) continue;
        dirty += sticky_fields(&t[i]) > 0;
    }
    vga_print_str("  free slots holding stale fields: ");
    vga_print_int(dirty);
    vga_print_str("   (read-only run)\n");
    return dirty;
}

/* launch() recycles the slot process_create() picks next and counts
 * what the new process inherited. One process, then we stop. */
static int atk_uninit_pcb_launch(void) {
    process_t *t = process_get_table();
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state == PROCESS_STATE_FREE) { slot = i; break; }
    if (slot < 0) return 0;

    /* Keep the attack bounded: replace any probe from an earlier run. */
    if (probe_pid) process_kill(probe_pid);

    /* uid 0: the slot quota is not the defence under test here. */
    int pid = process_create(PROBE_NAME, probe_worker, 0);
    if (pid < 0) return 0;
    probe_pid = (uint32_t)pid;

    int leaked = sticky_fields(&t[slot]);   /* same slot, new owner */
    vga_print_str("  inherited stale fields: ");
    vga_print_int(leaked);
    vga_print_str(leaked ? "  LEAK - the slot was not cleaned\n"
                         : "  clean - the slot was wiped first\n");
    return leaked;
}

/* cleanup() stops the probe we made, so the next test starts clean. */
static int atk_uninit_pcb_cleanup(void) {
    process_t *self = scheduler_get_current_process();
    int killed = 0;

    /* Never kill the caller: an attack must not take the shell with it. */
    if (probe_pid && (!self || self->pid != probe_pid)) {
        process_kill(probe_pid);
        killed = 1;
    }
    vga_print_str("  probe process stopped\n");
    probe_pid = 0;
    return killed;
}

static const attack_t atk = {
    "uninit_pcb",
    "a recycled process slot hands its previous owner's fields to the new process",
    INV_DISCLOSURE, SEV_HIGH,
    atk_uninit_pcb_simulate,
    atk_uninit_pcb_launch,
    atk_uninit_pcb_cleanup,
    "pcb_wipe"
};

void atk_uninit_pcb_register(void) {
    proc_security_register_attack(&atk);
}
