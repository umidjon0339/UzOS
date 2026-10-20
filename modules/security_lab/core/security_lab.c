/*
 * security_lab.c - Registry, event log and watchdog of the security lab.
 *
 * Uses: timer_ticks (device_management/timer) to time-stamp events,
 *       strcmp (lib/string) to look attacks up by id.
 */

#include "security_lab.h"
#include "string.h"
#include "timer.h"

/* ---------- Registry ---------- */

static const attack_t  *attacks[MAX_ATTACKS];
static const defence_t *defences[MAX_DEFENCES];
static int attack_count;
static int defence_count;

void proc_security_register_attack(const attack_t *a) {
    if (a && attack_count < MAX_ATTACKS)
        attacks[attack_count++] = a;
}

void proc_security_register_defence(const defence_t *d) {
    if (d && defence_count < MAX_DEFENCES)
        defences[defence_count++] = d;
}

int proc_security_attack_count(void)  { return attack_count; }
int proc_security_defence_count(void) { return defence_count; }

const attack_t *proc_security_attack(int index) {
    if (index < 0 || index >= attack_count) return 0;
    return attacks[index];
}

const defence_t *proc_security_defence(int index) {
    if (index < 0 || index >= defence_count) return 0;
    return defences[index];
}

const attack_t *proc_security_attack_by_id(const char *id) {
    for (int i = 0; i < attack_count; i++)
        if (strcmp(attacks[i]->id, id) == 0)
            return attacks[i];
    return 0;
}

const defence_t *proc_security_defence_by_id(const char *id) {
    for (int i = 0; i < defence_count; i++)
        if (strcmp(defences[i]->id, id) == 0)
            return defences[i];
    return 0;
}

int proc_security_verify(const char *defence_id) {
    const defence_t *d = proc_security_defence_by_id(defence_id);
    if (!d || !d->selftest)
        return -1;
    return d->selftest() ? 1 : 0;
}

/* ---------- Event log (ring buffer: the newest 32 events) ---------- */

static sec_event_t log_ring[SEC_LOG_SIZE];
static int log_head;            // where the next event goes
static int log_count;           // how many slots are filled
static uint32_t total_denied;

void sec_log(const char *defence, const char *detail) {
    sec_event_t *e = &log_ring[log_head];
    e->defence = defence;
    e->detail  = detail;
    e->tick    = timer_ticks;

    log_head = (log_head + 1) % SEC_LOG_SIZE;
    if (log_count < SEC_LOG_SIZE)
        log_count++;
    total_denied++;
}

int sec_log_count(void) { return log_count; }

const sec_event_t *sec_log_get(int index) {
    if (index < 0 || index >= log_count) return 0;
    int pos = (log_head - 1 - index + SEC_LOG_SIZE) % SEC_LOG_SIZE;
    return &log_ring[pos];
}

uint32_t sec_total_denied(void) { return total_denied; }

/* ---------- Watchdog ----------
 * `seclab attack <id> live` arms it with a time budget. If the attack is
 * still running when the budget runs out, the timer interrupt marks it
 * as overrun and logs it. It reports; it does not stop the attack. */

static uint32_t    wd_deadline;
static const char *wd_attack;
static int         wd_active;
static int         wd_fired;

void sec_watchdog_arm(uint32_t ticks, const char *attack_id) {
    wd_deadline = timer_ticks + ticks;
    wd_attack   = attack_id;
    wd_active   = 1;
    wd_fired    = 0;
}

void sec_watchdog_disarm(void) { wd_active = 0; }

void sec_watchdog_tick(void) {
    if (!wd_active || timer_ticks < wd_deadline)
        return;

    wd_active = 0;
    wd_fired  = 1;
    sec_log("watchdog", wd_attack ? wd_attack : "live attack");
}

int sec_watchdog_fired(void) { return wd_fired; }
const char *sec_watchdog_attack(void) { return wd_attack; }

/* ---------- Names ---------- */

const char *sev_name(sev_t s) {
    switch (s) {
        case SEV_INFO:     return "info";
        case SEV_LOW:      return "low";
        case SEV_MEDIUM:   return "medium";
        case SEV_HIGH:     return "high";
        case SEV_CRITICAL: return "CRITICAL";
    }
    return "?";
}

const char *inv_name(invariant_t i) {
    switch (i) {
        case INV_RESOURCE:    return "resource";
        case INV_IDENTITY:    return "identity";
        case INV_ISOLATION:   return "isolation";
        case INV_PRIVILEGE:   return "privilege";
        case INV_LIVENESS:    return "liveness";
        case INV_CONSISTENCY: return "consistency";
        case INV_DISCLOSURE:  return "disclosure";
        case INV_TRUST:       return "trust";
    }
    return "?";
}
