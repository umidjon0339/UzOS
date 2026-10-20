#ifndef SECURITY_LAB_H
#define SECURITY_LAB_H

/*
 * security_lab.h - Framework for attack / defence demonstrations.
 *
 * Every module can show how one of its guarantees is broken (an ATTACK)
 * and how the kernel stops it (a DEFENCE). This file is the shared part:
 * the registry of attacks and defences, the event log, and the watchdog.
 * The `seclab` shell command drives it.
 *
 *   attack.simulate()  only reads, reports what WOULD happen. Always safe.
 *   attack.launch()    really does it, in a small fixed amount.
 *   attack.cleanup()   undoes what launch() left behind (may be 0).
 *   defence.selftest() feeds the defence's rule bad inputs; 1 = it holds.
 *
 * A defence is a check the KERNEL calls at the exact place the attack
 * would strike. A defence that nothing calls is not a defence.
 *
 * (Names keep the old proc_security_ / sec_ prefix so other modules
 * did not have to change. They will be renamed when those are cleaned.)
 */

#include <stdint.h>

// How bad is it if the attack works?
typedef enum { SEV_INFO, SEV_LOW, SEV_MEDIUM, SEV_HIGH, SEV_CRITICAL } sev_t;

// Which guarantee ("invariant") does the attack break?
typedef enum {
    INV_RESOURCE,       // slots, PIDs, stacks and memory are finite
    INV_IDENTITY,       // a PID always names exactly one process
    INV_ISOLATION,      // processes cannot touch each other's memory
    INV_PRIVILEGE,      // user code cannot act with kernel rights
    INV_LIVENESS,       // no process can starve the others
    INV_CONSISTENCY,    // kernel tables stay consistent
    INV_DISCLOSURE,     // no leaking of stale or foreign data
    INV_TRUST           // input from outside is checked before use
} invariant_t;

typedef struct {
    const char *id;             // "fork_bomb"
    const char *what;           // one line: what the attack does
    invariant_t inv;
    sev_t       sev;
    int (*simulate)(void);      // read-only, returns number of findings
    int (*launch)(void);        // live, returns number of findings
    int (*cleanup)(void);       // undo, returns number of things reclaimed
    const char *defence;        // id of the defence that stops it
} attack_t;

typedef struct {
    const char *id;             // "slot_quota"
    const char *fix;            // one line: what the defence does
    const char *attack;         // id of the attack it stops
    int (*selftest)(void);      // 1 = the attack is blocked
} defence_t;

/* ---------- Registry ---------- */

#define MAX_ATTACKS  48
#define MAX_DEFENCES 48

// Register every module's attacks and defences. Called once at boot.
void proc_security_init(void);

void proc_security_register_attack(const attack_t *a);
void proc_security_register_defence(const defence_t *d);

int proc_security_attack_count(void);
int proc_security_defence_count(void);
const attack_t  *proc_security_attack(int index);
const defence_t *proc_security_defence(int index);
const attack_t  *proc_security_attack_by_id(const char *id);
const defence_t *proc_security_defence_by_id(const char *id);

// Run one defence's selftest: 1 = holds, 0 = bypassed, -1 = unknown id.
int proc_security_verify(const char *defence_id);

/* ---------- Event log: proof that a defence fired ---------- */

#define SEC_LOG_SIZE 32

typedef struct {
    const char *defence;        // who refused
    const char *detail;         // what was refused
    uint32_t    tick;           // when (timer ticks since boot)
} sec_event_t;

// A defence calls this every time it refuses something.
void sec_log(const char *defence, const char *detail);
int  sec_log_count(void);
const sec_event_t *sec_log_get(int index);  // 0 = newest
uint32_t sec_total_denied(void);            // refusals since boot

/* ---------- Watchdog: notices a live attack that runs too long ---------- */

void sec_watchdog_arm(uint32_t ticks, const char *attack_id);
void sec_watchdog_disarm(void);
void sec_watchdog_tick(void);               // called by the timer interrupt
int  sec_watchdog_fired(void);
const char *sec_watchdog_attack(void);

/* ---------- Names for printing ---------- */

const char *sev_name(sev_t s);
const char *inv_name(invariant_t i);

#endif
