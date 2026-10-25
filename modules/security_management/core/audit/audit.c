/*
 * audit.c - The audit log: a ring of events and a counter per defence.
 *
 *   audit_log(defence, detail)
 *     1. count it: the total and the defence's own counter
 *     2. ask audit_limit whether it may go into the ring
 *          yes: write it into the ring (over the oldest one if full)
 *          no:  only counted; the first time in a flood, write one
 *               note in audit_limit's name instead:
 *               "flood of descriptor_bounds, the rest only counted"
 *
 * audit_log() is called from normal code AND from interrupt handlers
 * (the mouse, the timer, CPU faults). An interrupt in the middle of an
 * update would leave the ring half-written, so interrupts stay off
 * while it changes.
 *
 * Uses: timer_ticks (device_management/timer) to time-stamp events,
 *       scheduler_get_current_process (process_management) for the PID
 *       and uid, def_audit_limit_allows (security/defence/audit_limit),
 *       cpu_irq_save / cpu_irq_restore (arch/x86/cpu), strcmp (lib/string),
 *       format (lib/format).
 */

#include "audit.h"
#include "def_audit_limit.h"
#include "timer.h"
#include "scheduler.h"
#include "cpu.h"
#include "string.h"
#include "format.h"

static audit_event_t ring[AUDIT_SIZE];
static int head;                // where the next event goes
static int count;               // how many slots are filled
static uint32_t lost;           // events overwritten because the ring was full

// A defence's counters, plus its allowance for this second (audit_limit).
typedef struct {
    audit_source_t counts;      // what audit_source() shows
    uint32_t window_start;      // tick when its current second began
    uint32_t stored;            // events it put into the ring since then
    int      noted;             // 1 = the flood note for this second is written
    char     note[56];          // its text; it lives here, so the ring may point to it
} source_t;

static source_t sources[AUDIT_SOURCES];
static int source_count;
static uint32_t total;
static uint32_t dropped;        // events only counted, not kept

// The counters of 'defence', made on its first event. 0 if all are taken.
static source_t *source_of(const char *defence) {
    for (int i = 0; i < source_count; i++)
        if (strcmp(sources[i].counts.defence, defence) == 0)
            return &sources[i];

    if (source_count == AUDIT_SOURCES)
        return 0;
    source_t *s = &sources[source_count++];
    s->counts.defence = defence;
    s->counts.refused = 0;
    s->counts.dropped = 0;
    s->window_start = timer_ticks;
    s->stored = 0;
    s->noted = 0;
    format(s->note, sizeof(s->note), "flood of %s, the rest only counted", defence);
    return s;
}

// Write one event into the ring.
static void store(const char *defence, const char *detail) {
    process_t *now = scheduler_get_current_process();
    audit_event_t *e = &ring[head];
    e->seq     = total;
    e->defence = defence;
    e->detail  = detail;
    e->tick    = timer_ticks;
    e->pid     = now ? now->pid : 0;
    e->uid     = now ? now->uid : 0;

    head = (head + 1) % AUDIT_SIZE;
    if (count < AUDIT_SIZE)
        count++;
    else
        lost++;                 // we just wrote over the oldest event
}

// Count one refusal of 'defence', kept or not.
static void count_one(source_t *s) {
    total++;
    if (s)
        s->counts.refused++;
}

void audit_log(const char *defence, const char *detail) {
    if (!defence) defence = "?";
    if (!detail)  detail  = "";

    uint32_t flags = cpu_irq_save();

    source_t *s = source_of(defence);
    count_one(s);

    if (s && timer_ticks - s->window_start >= AUDIT_WINDOW_TICKS) {
        s->window_start = timer_ticks;  // a new second, a new allowance
        s->stored = 0;
        s->noted = 0;
    }

    // DEFENCE audit_limit: may it take a slot in the ring?
    if (!s || def_audit_limit_allows(timer_ticks, s->window_start, s->stored)) {
        store(defence, detail);
        if (s)
            s->stored++;
    } else {
        s->counts.dropped++;
        dropped++;
        if (!s->noted) {                // once per flood and second
            s->noted = 1;
            count_one(source_of("audit_limit"));
            store("audit_limit", s->note);
        }
    }

    cpu_irq_restore(flags);
}

/* ---------- The ring ---------- */

int audit_count(void) { return count; }

uint32_t audit_lost(void) { return lost; }

int audit_snapshot(audit_event_t out[AUDIT_SIZE]) {
    uint32_t flags = cpu_irq_save();
    for (int i = 0; i < count; i++)
        out[i] = ring[(head - 1 - i + AUDIT_SIZE) % AUDIT_SIZE];
    int n = count;
    cpu_irq_restore(flags);
    return n;
}

/* ---------- The counters ---------- */

uint32_t audit_total(void) { return total; }

uint32_t audit_dropped(void) { return dropped; }

uint32_t audit_refused_by(const char *defence) {
    for (int i = 0; i < source_count; i++)
        if (strcmp(sources[i].counts.defence, defence) == 0)
            return sources[i].counts.refused;
    return 0;
}

int audit_source_count(void) { return source_count; }

const audit_source_t *audit_source(int index) {
    if (index < 0 || index >= source_count) return 0;
    return &sources[index].counts;
}
