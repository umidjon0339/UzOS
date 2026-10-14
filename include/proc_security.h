#ifndef PROC_SECURITY_H
#define PROC_SECURITY_H

#include <stdint.h>
#include "process.h"

/* ===================================================================
 * UzOS Process Security Lab
 * ===================================================================
 * An ATTACK shows one way process management can be broken.
 * A DEFENCE is the countermeasure, called by the kernel at the exact
 * place where the attack would strike.
 *
 * Every module follows the same contract:
 *
 *   simulate()  never changes kernel state. It reports what WOULD
 *               happen. Safe to run at any time.
 *   launch()    performs the real attack. Bounded by the watchdog.
 *   cleanup()   undoes whatever the attack left behind. May be 0.
 *   selftest()  returns 1 when the defence really blocks the attack.
 *
 * Recommended shape for a defence:
 *   1. a pure "policy" function   -> tiny, easy to read, easy to test
 *   2. the kernel calls it        -> the check is live, never dead code
 *
 * A defence that nothing calls is not a defence, it is a comment.
 * =================================================================== */

/* How damaging is the attack if it succeeds? */
typedef enum { SEV_INFO, SEV_LOW, SEV_MEDIUM, SEV_HIGH, SEV_CRITICAL } sev_t;

/* Which guarantee of process management does the attack break? */
typedef enum {
    INV_RESOURCE,    /* finite PIDs, slots and stacks           */
    INV_IDENTITY,    /* a PID always names exactly one process  */
    INV_ISOLATION,   /* processes cannot touch each other       */
    INV_PRIVILEGE,   /* user code cannot act with kernel rights */
    INV_LIVENESS,    /* no process can starve the others        */
    INV_CONSISTENCY, /* kernel tables stay consistent           */
    INV_DISCLOSURE,  /* no leaking of stale or foreign data     */
    INV_TRUST        /* input from outside is validated         */
} invariant_t;

typedef struct {
    const char *id;         /* "fork_bomb"                      */
    const char *what;       /* one line: what the attack does   */
    invariant_t inv;
    sev_t       sev;
    int (*simulate)(void);  /* safe   : returns finding count   */
    int (*launch)(void);    /* live   : returns finding count   */
    int (*cleanup)(void);   /* undo   : returns reclaimed count */
    const char *defence;    /* id of the paired defence         */
} attack_t;

typedef struct {
    const char *id;         /* "slot_quota"                     */
    const char *fix;        /* one line: what it does           */
    const char *attack;     /* id of the paired attack          */
    int (*selftest)(void);  /* 1 = the attack is blocked        */
} defence_t;

/* ---- registry ---------------------------------------------------- */
#define MAX_ATTACKS  16
#define MAX_DEFENCES 16

void proc_security_register_attack(const attack_t *a);
void proc_security_register_defence(const defence_t *d);
void proc_security_init(void);          /* called once from kernel_main */

int proc_security_attack_count(void);
int proc_security_defence_count(void);
const attack_t  *proc_security_attack(int index);
const defence_t *proc_security_defence(int index);
const attack_t  *proc_security_attack_by_id(const char *id);
const defence_t *proc_security_defence_by_id(const char *id);

/* Run one defence's own selftest.
 * returns 1 = blocked, 0 = bypassed, -1 = no such defence. */
int proc_security_verify(const char *defence_id);
int proc_security_verify_all(void);     /* returns number of bypasses */

/* ---- event log: the evidence that a defence fired ---------------- */
#define SEC_LOG_SIZE 32

typedef struct {
    const char *defence;
    const char *detail;
    uint32_t    tick;
} sec_event_t;

void sec_log(const char *defence, const char *detail);
int  sec_log_count(void);
const sec_event_t *sec_log_get(int index);  /* 0 = newest */
uint32_t sec_total_denied(void);

/* ---- watchdog: stops a live attack from hanging the OS ----------- */
void sec_watchdog_arm(uint32_t ticks, const char *attack_id);
void sec_watchdog_disarm(void);             /* attack finished in time */
void sec_watchdog_tick(void);               /* called from the PIT */
int  sec_watchdog_fired(void);
const char *sec_watchdog_attack(void);

/* ---- defence entry points called by the kernel ------------------- */
int  def_slot_quota_check(uint32_t uid);                  /* create     */
void def_pcb_wipe(process_t *p);                          /* create     */
void def_release_child_slot(process_t *child);            /* fork fail  */
int  def_kill_allowed(uint32_t caller, uint32_t target);  /* kill       */
int  def_self_kill_policy(uint32_t caller, uint32_t target); /* 1 = free now */
void def_child_release(process_t *parent);                /* exit/reap  */
int  def_is_reapable(process_t *p);                       /* tick       */
void def_reap_zombies(void);                              /* tick       */
int  def_stack_guard_check(process_t *p);                 /* switch     */
int  def_user_stack_ok(uint32_t esp);                     /* ring 3     */
int  def_pid_bounds_check(uint32_t pid);                  /* create     */
int  def_elf_validate(const void *ehdr, const void *phdrs, int count);

/* pretty names for the shell driver */
const char *sev_name(sev_t s);
const char *inv_name(invariant_t i);

#endif
