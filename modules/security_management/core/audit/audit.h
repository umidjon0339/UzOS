#ifndef AUDIT_H
#define AUDIT_H

/*
 * audit.h - The audit log: proof that a defence fired, and on whom.
 *
 * Every time a defence refuses something it calls audit_log(). The log
 * notes what was refused, when, and which process (and so which user)
 * was running. Two kinds of memory:
 *
 *   the ring      the newest AUDIT_SIZE events, with every detail.
 *                 When it is full, the oldest event is overwritten
 *                 (counted in audit_lost()).
 *   the counters  one per defence: how often it refused since boot.
 *                 They never forget, even when the ring does.
 *
 * One defence cannot fill the whole ring: after 10 events in a second,
 * the rest of its events are only counted (defence audit_limit). So a
 * flood of cheap refusals cannot push the important one out.
 *
 * There is no way to clear the log: a log that can be wiped is no proof.
 */

#include <stdint.h>

#define AUDIT_SIZE    32        // events kept in the ring
#define AUDIT_SOURCES 64        // defences with their own counter

typedef struct {
    uint32_t    seq;            // the seq-th refusal since boot (= audit_total() then)
    const char *defence;        // who refused
    const char *detail;         // what was refused
    uint32_t    tick;           // when (timer ticks since boot)
    uint32_t    pid;            // who was running (0 = no process yet)
    uint32_t    uid;            // ...and as which user
} audit_event_t;

typedef struct {
    const char *defence;
    uint32_t    refused;        // refusals since boot
    uint32_t    dropped;        // ...of which only counted, not kept (audit_limit)
} audit_source_t;

// A defence calls this every time it refuses something. Safe to call
// from an interrupt handler.
void audit_log(const char *defence, const char *detail);

/* ---------- The ring ---------- */
int audit_count(void);                      // events kept (at most AUDIT_SIZE)
uint32_t audit_lost(void);                  // events overwritten since boot

// Copy the ring into 'out', newest first; returns how many. The copy is
// taken with interrupts off, so it cannot change while you print it.
int audit_snapshot(audit_event_t out[AUDIT_SIZE]);

/* ---------- The counters ---------- */
uint32_t audit_total(void);                 // refusals since boot, all defences
uint32_t audit_dropped(void);               // events only counted (audit_limit)
uint32_t audit_refused_by(const char *defence);
int audit_source_count(void);
const audit_source_t *audit_source(int index);  // in order of first refusal

#endif
