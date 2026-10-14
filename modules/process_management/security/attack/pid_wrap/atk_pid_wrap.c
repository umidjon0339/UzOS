#include "proc_security.h"
#include "process.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: PID wrap
 * ===================================================================
 * Break INV_IDENTITY: a PID must name exactly one process.
 *
 * PIDs come from a counter that only goes up. Nothing stopped it
 * reaching the end of its 32-bit range and wrapping to a small number
 * still in use, so two live processes would share a PID and
 * process_get_by_pid() would quietly return the wrong one.
 *
 * The wrap is four billion creations away, so launch() cannot run it.
 * Instead it probes the guard at the PIDs a wrap would land on.
 * =================================================================== */

#define PID_WRAP_EDGE 0xFFFFFFFFu  /* last value before the counter wraps  */
#define GUARD_LIMIT   0xFFFFFF00u  /* where pid_bounds stops handing PIDs  */
#define SMALL_PID     32u          /* a wrap lands in this low range first */

/* Count the live processes that a wrapped counter would collide with. */
static int count_live_small_pids(void) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state != PROCESS_STATE_FREE && t[i].pid < SMALL_PID) n++;
    return n;
}

/* simulate() is read-only: arithmetic only, never the logging guard. */
static int atk_pid_wrap_simulate(void) {
    process_t *t = process_get_table();
    uint32_t top = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state != PROCESS_STATE_FREE && t[i].pid > top) top = t[i].pid;

    uint32_t to_guard = (top < GUARD_LIMIT) ? (GUARD_LIMIT - top) : 0u;
    int reuse = count_live_small_pids();

    vga_print_str("  highest live PID now   : 0x");
    vga_print_hex(top);
    vga_print_str("\n  creations before guard : ");
    vga_print_int((int)(to_guard / 1000000u));
    vga_print_str(" million (~50 days at 1000/s)\n");
    vga_print_str("  live PIDs a wrap reuses: ");
    vga_print_int(reuse);
    vga_print_str("\n  (nothing created - simulate() is read-only)\n");
    return reuse;
}

/* launch() creates and kills nothing; it only probes the guard. */
static int atk_pid_wrap_launch(void) {
    static const uint32_t probes[] = {
        1u,               /* a normal, healthy PID             */
        GUARD_LIMIT - 1u, /* the last PID the guard allows     */
        GUARD_LIMIT,      /* the guard must refuse from here   */
        PID_WRAP_EDGE,    /* the counter would wrap after this */
        0u                /* PID 0 always means "no process"   */
    };
    int n = (int)(sizeof(probes) / sizeof(probes[0]));
    int refused = 0;
    vga_print_str("  cannot run 4 billion creations - probing instead\n");

    for (int i = 0; i < n; i++) {
        int allowed = def_pid_bounds_check(probes[i]);
        if (!allowed) refused++;
        vga_print_str("  pid 0x");
        vga_print_hex(probes[i]);
        vga_print_str(allowed ? " accepted\n" : " refused by pid_bounds\n");
    }
    /* The guard forgoes 0xFFFFFF00..0xFFFFFFFF: 256 spare PIDs. */
    vga_print_str("  wrap distance: 2^32 creations, 256 PIDs spare\n");
    return refused;
}

static const attack_t atk = {
    "pid_wrap",
    "the PID counter can wrap and collide with a live PID",
    INV_IDENTITY,
    SEV_HIGH,
    atk_pid_wrap_simulate,
    atk_pid_wrap_launch,
    0,               /* launch creates nothing, so nothing to undo */
    "pid_bounds"
};

void atk_pid_wrap_register(void) {
    proc_security_register_attack(&atk);
}
