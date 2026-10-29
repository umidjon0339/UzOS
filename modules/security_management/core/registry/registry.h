#ifndef REGISTRY_H
#define REGISTRY_H

/*
 * registry.h - The list of every attack and defence in UzOS.
 *
 * Every module can show how one of its guarantees is broken (an ATTACK)
 * and how the kernel stops it (a DEFENCE). The pairs live in each
 * module (modules/<module>/security/); this registry only keeps a
 * pointer to each, so the `seclab` command can find them.
 *
 *   attack.simulate()  only reads, reports what WOULD happen. Always safe.
 *   attack.launch()    really does it, in a small fixed amount.
 *   attack.cleanup()   undoes what launch() left behind (may be 0).
 *   defence.selftest() feeds the defence's rule bad inputs; 1 = it holds.
 *
 * A defence is a check the KERNEL calls at the exact place the attack
 * would strike, and it calls audit_log() (core/audit) when it refuses.
 * A defence that nothing calls is not a defence.
 */

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
    INV_TRUST,          // input from outside is checked before use
    INV_INTEGRITY,      // kernel code and the audit trail stay unchanged
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

#define REGISTRY_MAX_ATTACKS  80
#define REGISTRY_MAX_DEFENCES 80

// Each pair's <id>_register() function calls these (see core/catalog).
void registry_add_attack(const attack_t *a);
void registry_add_defence(const defence_t *d);

int registry_attack_count(void);
int registry_defence_count(void);
const attack_t  *registry_attack(int index);
const defence_t *registry_defence(int index);
const attack_t  *registry_find_attack(const char *id);
const defence_t *registry_find_defence(const char *id);

// Run one defence's selftest: 1 = holds, 0 = bypassed, -1 = unknown id.
int registry_verify(const char *defence_id);

// Names for printing.
const char *sev_name(sev_t s);
const char *inv_name(invariant_t i);

#endif
