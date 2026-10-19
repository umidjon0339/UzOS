#include "proc_security.h"
#include "proc_security_internal.h"
#include "timer.h"

// Registry for attacks and defences
static const attack_t  *attacks[MAX_ATTACKS];
static const defence_t *defences[MAX_DEFENCES];
static int attack_count  = 0;
static int defence_count = 0;

void proc_security_register_attack(const attack_t *a) {
    if (a && attack_count < MAX_ATTACKS) attacks[attack_count++] = a;
}

void proc_security_register_defence(const defence_t *d) {
    if (d && defence_count < MAX_DEFENCES) defences[defence_count++] = d;
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

// String comparison helper
static int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

const attack_t *proc_security_attack_by_id(const char *id) {
    for (int i = 0; i < attack_count; i++)
        if (str_equal(attacks[i]->id, id)) return attacks[i];
    return 0;
}

const defence_t *proc_security_defence_by_id(const char *id) {
    for (int i = 0; i < defence_count; i++)
        if (str_equal(defences[i]->id, id)) return defences[i];
    return 0;
}

int proc_security_verify(const char *defence_id) {
    const defence_t *d = proc_security_defence_by_id(defence_id);
    if (!d || !d->selftest) return -1;
    return d->selftest() ? 1 : 0;
}

int proc_security_verify_all(void) {
    int bypassed = 0;
    for (int i = 0; i < defence_count; i++) {
        if (!defences[i]->selftest || !defences[i]->selftest()) bypassed++;
    }
    return bypassed;
}

// Event log ring buffer
static sec_event_t log_ring[SEC_LOG_SIZE];
static int log_head = 0;
static int log_total = 0;
static uint32_t total_denied = 0;

void sec_log(const char *defence, const char *detail) {
    sec_event_t *e = &log_ring[log_head];
    e->defence = defence;
    e->detail  = detail;
    e->tick    = timer_ticks;

    log_head = (log_head + 1) % SEC_LOG_SIZE;
    if (log_total < SEC_LOG_SIZE) log_total++;
    total_denied++;
}

int sec_log_count(void) { return log_total; }

const sec_event_t *sec_log_get(int index) {
    if (index < 0 || index >= log_total) return 0;
    int pos = (log_head - 1 - index + SEC_LOG_SIZE) % SEC_LOG_SIZE;
    return &log_ring[pos];
}

uint32_t sec_total_denied(void) { return total_denied; }

// Attack Watchdog
static uint32_t    wd_deadline = 0;
static const char *wd_attack   = 0;
static int         wd_active   = 0;
static int         wd_fired    = 0;

void sec_watchdog_arm(uint32_t ticks, const char *attack_id) {
    wd_deadline = timer_ticks + ticks;
    wd_attack   = attack_id;
    wd_active   = 1;
    wd_fired    = 0;
}

void sec_watchdog_disarm(void) { wd_active = 0; }

void sec_watchdog_tick(void) {
    if (!wd_active) return;
    if (timer_ticks < wd_deadline) return;

    wd_active = 0;
    wd_fired  = 1;
    sec_log("watchdog", wd_attack ? wd_attack : "live attack");
}

int sec_watchdog_fired(void) { return wd_fired; }
const char *sec_watchdog_attack(void) { return wd_attack; }

// Pretty names
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

// Register security demonstrations across kernel subsystems.
void proc_security_init(void) {
    atk_fork_bomb_register();
    atk_kill_init_register();
    atk_stack_overflow_register();
    atk_zombie_leak_register();
    atk_pid_wrap_register();
    atk_uninit_pcb_register();

    def_slot_quota_register();
    def_kill_guard_register();
    def_stack_guard_register();
    def_reaper_register();
    def_pid_bounds_register();
    def_pcb_wipe_register();

    atk_user_ptr_wrap_register();
    atk_mmap_length_wrap_register();
    atk_pmm_free_oob_register();

    def_user_ptr_bounds_register();
    def_mmap_page_count_register();
    def_pmm_free_bounds_register();

    atk_path_traversal_register();
    atk_file_offset_wrap_register();
    def_path_component_register();
    def_file_range_register();

    atk_timer_divide_by_zero_register();
    atk_mouse_coordinate_escape_register();
    def_timer_frequency_register();
    def_mouse_bounds_register();

    atk_invalid_descriptor_register();
    def_descriptor_bounds_register();
    atk_oversized_transfer_register();
    def_transfer_bounds_register();
}

int def_elf_validate(const void *ehdr, const void *phdrs, int count) {
    (void)ehdr; (void)phdrs; (void)count;
    return 1;
}

int def_user_stack_ok(uint32_t esp) {
    (void)esp;
    return 1;
}

void def_child_release(process_t *parent) {
    (void)parent;
}
