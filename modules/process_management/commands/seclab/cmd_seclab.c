#include "proc_security.h"
#include "process.h"
#include "vga.h"
#include "timer.h"

/* ===================================================================
 * `seclab` - one shell command that drives the whole security lab.
 *
 *   seclab list                 every attack, worst first
 *   seclab info <id>            what it does and how it is stopped
 *   seclab attack <id> [live]   simulate (safe) or launch (real)
 *   seclab defence <attack|defence id>
 *   seclab verify <defence id>  run that defence's own selftest
 *   seclab verify-all           run every selftest
 *   seclab log                  the most recent refusals
 *   seclab status               counters and watchdog state
 *   seclab repair               let every attack undo itself
 *
 * The lab is safe to explore: `attack` without the word "live" never
 * changes anything.
 * =================================================================== */

/* Watchdog budget for a live run: 100 Hz timer, so 200 ticks = ~2 s. */
#define LIVE_BUDGET_TICKS 200

static int same(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Pull one space separated word off the front of s. Returns the rest. */
static const char *next_word(const char *s, char *out, int max) {
    while (*s == ' ') s++;

    int i = 0;
    while (*s && *s != ' ' && i < max - 1) out[i++] = *s++;
    out[i] = '\0';

    while (*s == ' ') s++;
    return s;
}

static void print_attack_line(const attack_t *a) {
    vga_print_str("  ");
    vga_print_str(a->id);
    vga_print_str("\n     breaks ");
    vga_print_str(inv_name(a->inv));
    vga_print_str(" | severity ");
    vga_print_str(sev_name(a->sev));
    vga_print_str(" | blocked by ");
    vga_print_str(a->defence ? a->defence : "(none)");
    vga_print_str("\n");
}

/* ---- subcommands ------------------------------------------------- */

static void do_list(void) {
    int n = proc_security_attack_count();

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\nProcess Security Lab - ");
    vga_print_int(n);
    vga_print_str(" attacks, ");
    vga_print_int(proc_security_defence_count());
    vga_print_str(" defences\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int i = 0; i < n; i++) print_attack_line(proc_security_attack(i));

    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_print_str("\n  seclab attack <id>        simulate (safe)\n");
    vga_print_str("  seclab attack <id> live   run it for real\n");
    vga_print_str("  seclab verify-all         test every defence\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void do_info(const char *id) {
    const attack_t *a = proc_security_attack_by_id(id);
    if (!a) {
        vga_print_str("No attack called '");
        vga_print_str(id);
        vga_print_str("'. Try: seclab list\n");
        return;
    }

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\nATTACK ");
    vga_print_str(a->id);
    vga_print_str("\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("  what      : ");
    vga_print_str(a->what);
    vga_print_str("\n  breaks    : ");
    vga_print_str(inv_name(a->inv));
    vga_print_str("\n  severity  : ");
    vga_print_str(sev_name(a->sev));
    vga_print_str("\n");

    const defence_t *d = a->defence ? proc_security_defence_by_id(a->defence) : 0;
    if (d) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print_str("\nDEFENCE ");
        vga_print_str(d->id);
        vga_print_str("\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print_str("  fix       : ");
        vga_print_str(d->fix);
        vga_print_str("\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("\n  no defence registered for this attack\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

static void do_attack(const char *id, const char *mode) {
    const attack_t *a = proc_security_attack_by_id(id);
    if (!a) {
        vga_print_str("No attack called '");
        vga_print_str(id);
        vga_print_str("'. Try: seclab list\n");
        return;
    }

    int live = same(mode, "live");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n[");
    vga_print_str(live ? "LIVE" : "SIMULATE");
    vga_print_str("] ");
    vga_print_str(a->id);
    vga_print_str("\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    if (!live) {
        int findings = a->simulate ? a->simulate() : 0;
        vga_print_str("  findings  : ");
        vga_print_int(findings);
        vga_print_str("\n  (read-only run - nothing was changed)\n");
        return;
    }

    /* A live run is given a bounded amount of time and its refusals
     * are counted, so we can tell an attack that worked from one that
     * was blocked. */
    uint32_t denied_before = sec_total_denied();
    sec_watchdog_arm(LIVE_BUDGET_TICKS, a->id);

    int findings = a->launch ? a->launch() : 0;

    /* Ask whether it fired, then stand the watchdog down. */
    int overran = sec_watchdog_fired();
    sec_watchdog_disarm();

    uint32_t denied_now = sec_total_denied() - denied_before;

    vga_print_str("  findings  : ");
    vga_print_int(findings);
    vga_print_str("\n  refusals  : ");
    vga_print_int((int)denied_now);
    vga_print_str("\n");

    if (overran) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("  WATCHDOG: the attack overran its time budget\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    if (denied_now > 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print_str("  verdict   : BLOCKED - the defence refused it\n");
    } else {
        /* Not every defence says no. Some let the operation through and
         * then clean up after it (the reaper, the fork cleanup), so the
         * refusals counter legitimately stays at zero. Fall back to the
         * paired defence's own selftest instead of claiming the attack
         * got through. */
        const defence_t *d = a->defence ? proc_security_defence_by_id(a->defence) : 0;
        int held = (d && d->selftest && d->selftest());

        vga_set_color(held ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_LIGHT_RED,
                      VGA_COLOR_BLACK);
        if (held) {
            vga_print_str("  verdict   : DEFENCE HOLDS - it cleans up rather than refusing\n");
        } else {
            vga_print_str("  verdict   : NOT BLOCKED - no refusal and the selftest failed\n");
        }
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("  run 'seclab repair' to clean up\n");
}

static void do_defence(const char *id) {
    const defence_t *d = proc_security_defence_by_id(id);
    if (!d) {
        const attack_t *a = proc_security_attack_by_id(id);
        if (a && a->defence) d = proc_security_defence_by_id(a->defence);
    }
    if (!d) {
        vga_print_str("No defence matched '");
        vga_print_str(id);
        vga_print_str("'.\n");
        return;
    }

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("\nDEFENCE ");
    vga_print_str(d->id);
    vga_print_str("\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("  fix       : ");
    vga_print_str(d->fix);
    vga_print_str("\n  counters  : ");
    vga_print_str(d->attack ? d->attack : "(none)");
    vga_print_str("\n");
}

static void do_verify(const char *id) {
    int r = proc_security_verify(id);

    if (r < 0) {
        vga_print_str("No defence called '");
        vga_print_str(id);
        vga_print_str("'.\n");
        return;
    }

    vga_print_str("  ");
    vga_print_str(id);
    vga_print_str(" ... ");
    if (r) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print_str("BLOCKED (selftest passed)\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("BYPASSED (selftest failed)\n");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void do_verify_all(void) {
    int total = proc_security_defence_count();
    int bypassed = 0;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\nRunning every defence selftest\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int i = 0; i < total; i++) {
        const defence_t *d = proc_security_defence(i);
        if (!d->selftest) continue;

        int ok = d->selftest();
        if (!ok) bypassed++;

        vga_print_str("  ");
        vga_print_str(d->id);
        vga_print_str(" ... ");
        if (ok) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_print_str("BLOCKED\n");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print_str("BYPASSED\n");
        }
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    vga_print_str("\n  ");
    vga_print_int(total - bypassed);
    vga_print_str(" of ");
    vga_print_int(total);
    vga_print_str(" defences hold\n");
}

static void do_log(void) {
    int n = sec_log_count();

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\nSecurity events (newest first)\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    if (n == 0) {
        vga_print_str("  none yet - no defence has refused anything\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        const sec_event_t *e = sec_log_get(i);
        if (!e) break;

        vga_print_str("  [tick ");
        vga_print_int((int)e->tick);
        vga_print_str("] ");
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str(e->defence);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print_str(" - ");
        vga_print_str(e->detail);
        vga_print_str("\n");
    }
}

static void do_status(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\nProcess Security Lab status\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_print_str("  attacks registered : ");
    vga_print_int(proc_security_attack_count());
    vga_print_str("\n  defences registered: ");
    vga_print_int(proc_security_defence_count());
    vga_print_str("\n  refusals logged    : ");
    vga_print_int((int)sec_total_denied());
    vga_print_str("\n  watchdog           : ");
    if (sec_watchdog_fired()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("FIRED on ");
        vga_print_str(sec_watchdog_attack() ? sec_watchdog_attack() : "?");
    } else {
        vga_print_str("idle");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("\n");

    /* Free process slots: the resource most attacks go after. */
    process_t *t = process_get_table();
    int free_slots = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state == PROCESS_STATE_FREE) free_slots++;

    vga_print_str("  free process slots : ");
    vga_print_int(free_slots);
    vga_print_str(" / ");
    vga_print_int(MAX_PROCESSES);
    vga_print_str("\n");
}

static void do_repair(void) {
    vga_print_str("\nAsking every attack to undo itself\n");

    for (int i = 0; i < proc_security_attack_count(); i++) {
        const attack_t *a = proc_security_attack(i);
        if (!a->cleanup) continue;
        a->cleanup();
    }

    /* Anything left as a zombie is reclaimed on the next tick. */
    def_reap_zombies();
    vga_print_str("  done\n");
}

/* ---- entry point ------------------------------------------------- */

void cmd_seclab(const char *args) {
    char sub[24];
    char id[32];
    char mode[16];

    const char *rest = next_word(args, sub, sizeof(sub));

    if (sub[0] == '\0' || same(sub, "list")) {
        do_list();
        return;
    }

    if (same(sub, "status")) { do_status();      return; }
    if (same(sub, "log"))    { do_log();         return; }
    if (same(sub, "repair")) { do_repair();      return; }

    if (same(sub, "verify")) {
        next_word(rest, id, sizeof(id));
        if (same(id, "all")) do_verify_all();
        else                 do_verify(id);
        return;
    }

    if (same(sub, "info")) {
        next_word(rest, id, sizeof(id));
        do_info(id);
        return;
    }

    if (same(sub, "defence")) {
        next_word(rest, id, sizeof(id));
        do_defence(id);
        return;
    }

    if (same(sub, "attack")) {
        rest = next_word(rest, id, sizeof(id));
        next_word(rest, mode, sizeof(mode));
        do_attack(id, mode);
        return;
    }

    vga_print_str("Usage: seclab list | info <id> | attack <id> [live]\n");
    vga_print_str("       seclab defence <id> | verify <id|all>\n");
    vga_print_str("       seclab log | status | repair\n");
}
