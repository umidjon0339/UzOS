/*
 * atk_log_flood.c - ATTACK log_flood: hide a refusal under a flood.
 *
 * Breaks:     integrity - the audit log still shows what happened
 * Stopped by: audit_limit (security/defence/audit_limit)
 *
 * The audit ring keeps the newest 32 events. So:
 *   1. do the thing you want hidden: here, try to kill the idle task.
 *      kill_guard refuses and logs it: that event is the evidence.
 *   2. make the kernel refuse something cheap, again and again, until
 *      the ring is written over. close(-1) costs nothing, and any user
 *      may call it (from ring 3 as well).
 * Whoever reads `audit` afterwards sees 32 harmless close() refusals,
 * and not the kill.
 *
 * simulate(): explain how many events it takes.
 * launch():   1. process_kill(1)           one kill_guard event
 *             2. sys_close(-1), FLOOD times  descriptor_bounds events
 *             3. look in the ring: is the kill_guard event still there?
 *
 * Uses: audit_* (core/audit), process_kill (process_management),
 *       sys_close (io_management), ui_* (ui module).
 */

#include "registry.h"
#include "audit.h"
#include "process.h"
#include "sys_close.h"
#include "ui.h"

#define FLOOD      (AUDIT_SIZE * 2)     // twice the ring: surely enough
#define IDLE_PID   1
#define BAD_FD     -1

static audit_event_t ring[AUDIT_SIZE];  // a snapshot to search

// Is the event with this sequence number still in the ring?
static int still_logged(uint32_t seq) {
    int n = audit_snapshot(ring);
    for (int i = 0; i < n; i++)
        if (ring[i].seq == seq)
            return 1;
    return 0;
}

static int simulate(void) {
    ui_printf("  the audit ring keeps the newest %d events\n", AUDIT_SIZE);
    ui_printf("  1. one refusal to hide (kill the idle task)\n");
    ui_printf("  2. %d more cheap refusals: close(-1), allowed for any user\n", AUDIT_SIZE);
    ui_printf("  without a limit, event 1 is written over and gone\n");
    return 1;
}

static int launch(void) {
    // 1. The evidence.
    process_kill(IDLE_PID);
    uint32_t evidence = audit_total();      // its sequence number
    if (!still_logged(evidence)) {
        ui_printf("  kill_guard did not log the kill; nothing to hide\n");
        return 0;
    }
    ui_printf("  1. kill PID %d: refused, logged as event #%u\n", IDLE_PID, evidence);

    // 2. The flood.
    uint32_t before = audit_refused_by("descriptor_bounds");
    uint32_t dropped_before = audit_dropped();
    for (int i = 0; i < FLOOD; i++)
        sys_close(BAD_FD);
    ui_printf("  2. close(-1) %d times: %u refusals by descriptor_bounds\n",
              FLOOD, audit_refused_by("descriptor_bounds") - before);

    // 3. Did the evidence survive?
    int hidden = !still_logged(evidence);
    ui_printf("  3. event #%u: ", evidence);
    ui_color(hidden ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", hidden ? "GONE - the flood wrote over it"
                             : "still in the log");
    ui_reset();
    ui_printf("     only counted, not kept: %u (see `audit sum`)\n",
              audit_dropped() - dropped_before);
    return hidden;
}

static const attack_t attack = {
    "log_flood",
    "push a refusal out of the audit log with a flood of cheap ones",
    INV_INTEGRITY, SEV_MEDIUM,
    simulate, launch, 0,        // nothing to undo: refusals change nothing
    "audit_limit"
};

void atk_log_flood_register(void) {
    registry_add_attack(&attack);
}
