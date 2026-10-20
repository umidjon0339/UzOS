/*
 * cmd_seclab.c - `seclab`: explore the attacks and defences.
 *
 *   seclab list                  every attack and its defence
 *   seclab info <attack>         what it does and what stops it
 *   seclab attack <id>           simulate: read-only, always safe
 *   seclab attack <id> live      really run it (small, bounded; root only)
 *   seclab defence <id>          show a defence (by its id or its attack's)
 *   seclab verify <id> | all     run defence selftests
 *   seclab log                   newest refusals first
 *   seclab status                counters, watchdog, free process slots
 *   seclab repair                let every attack undo itself (root only)
 *
 * Uses: the lab registry and log (security_lab/core), the process table
 *       for `status`, the reaper for `repair` and process_caller_uid
 *       (process_management), UID_ROOT (user_management), arg_next
 *       (lib/args), ui_* (ui module).
 */

#include "security_lab_commands.h"
#include "security_lab.h"
#include "shell.h"
#include "string.h"
#include "args.h"
#include "ui.h"
#include "process.h"
#include "def_reaper.h"
#include "users.h"

// Time budget for a live attack: 200 ticks at 100 Hz = 2 seconds.
#define LIVE_BUDGET_TICKS 200

/* ---------- Small helpers ---------- */

static const attack_t *find_attack_or_complain(const char *id) {
    const attack_t *a = proc_security_attack_by_id(id);
    if (!a) {
        ui_start(UI_ERROR, "seclab");
        ui_printf("no attack called '%s' (try: seclab list)", id);
        ui_end();
    }
    return a;
}

// Print "  <id> ... BLOCKED" or "BYPASSED". Returns 1 if it holds.
static int print_selftest(const defence_t *d) {
    int ok = d->selftest && d->selftest();

    vga_print_str("  ");
    vga_print_str(d->id);
    vga_print_str(" ... ");
    ui_color(ok ? UI_GOOD : UI_BAD);
    vga_print_str(ok ? "BLOCKED\n" : "BYPASSED\n");
    ui_reset();
    return ok;
}

/* ---------- Subcommands ---------- */

static void do_list(void) {
    ui_color(UI_TITLE);
    vga_print_str("\nSystem Security Lab - ");
    vga_print_int(proc_security_attack_count());
    vga_print_str(" attacks, ");
    vga_print_int(proc_security_defence_count());
    vga_print_str(" defences\n");
    ui_reset();

    for (int i = 0; i < proc_security_attack_count(); i++) {
        const attack_t *a = proc_security_attack(i);
        vga_print_str("  ");
        vga_print_str(a->id);
        vga_print_str(" -> ");
        vga_print_str(a->defence ? a->defence : "(none)");
        vga_print_str("  [");
        vga_print_str(inv_name(a->inv));
        vga_print_str(", ");
        vga_print_str(sev_name(a->sev));
        vga_print_str("]\n");
    }

    ui_color(UI_DIM);
    vga_print_str("\n  seclab attack <id>        simulate (safe)\n");
    vga_print_str("  seclab attack <id> live   run it for real\n");
    vga_print_str("  seclab verify all         test every defence\n");
    ui_reset();
}

static void do_info(const char *id) {
    const attack_t *a = find_attack_or_complain(id);
    if (!a) return;

    ui_color(UI_TITLE);
    vga_print_str("\nATTACK ");
    vga_print_str(a->id);
    ui_reset();
    vga_print_str("\n  what     : ");
    vga_print_str(a->what);
    vga_print_str("\n  breaks   : ");
    vga_print_str(inv_name(a->inv));
    vga_print_str("\n  severity : ");
    vga_print_str(sev_name(a->sev));
    vga_print_str("\n");

    const defence_t *d = a->defence ? proc_security_defence_by_id(a->defence) : 0;
    if (!d) {
        ui_color(UI_BAD);
        vga_print_str("\n  no defence registered for this attack\n");
        ui_reset();
        return;
    }

    ui_color(UI_GOOD);
    vga_print_str("\nDEFENCE ");
    vga_print_str(d->id);
    ui_reset();
    vga_print_str("\n  fix      : ");
    vga_print_str(d->fix);
    vga_print_str("\n");
}

static void do_attack(const char *id, const char *mode) {
    const attack_t *a = find_attack_or_complain(id);
    if (!a) return;

    int live = strcmp(mode, "live") == 0;

    ui_color(UI_TITLE);
    vga_print_str(live ? "\n[LIVE] " : "\n[SIMULATE] ");
    vga_print_str(a->id);
    vga_print_str("\n");
    ui_reset();

    if (!live) {
        int findings = a->simulate ? a->simulate() : 0;
        vga_print_str("  findings : ");
        vga_print_int(findings);
        vga_print_str("\n  (read-only run - nothing was changed)\n");
        return;
    }

    // Count refusals during the run: more than zero = a defence said no.
    uint32_t denied_before = sec_total_denied();
    sec_watchdog_arm(LIVE_BUDGET_TICKS, a->id);

    int findings = a->launch ? a->launch() : 0;

    int overran = sec_watchdog_fired();
    sec_watchdog_disarm();
    uint32_t refused = sec_total_denied() - denied_before;

    vga_print_str("  findings : ");
    vga_print_int(findings);
    vga_print_str("\n  refusals : ");
    vga_print_int((int)refused);
    vga_print_str("\n");

    if (overran) {
        ui_color(UI_BAD);
        vga_print_str("  WATCHDOG: the attack overran its time budget\n");
    }

    if (refused > 0) {
        ui_color(UI_GOOD);
        vga_print_str("  verdict  : BLOCKED - the defence refused it\n");
    } else {
        // Some defences never need to refuse (the reaper cleans up, the
        // null page is fixed at boot), so zero refusals is not proof of
        // failure: ask the defence's selftest.
        const defence_t *d = a->defence ? proc_security_defence_by_id(a->defence) : 0;
        int holds = d && d->selftest && d->selftest();

        ui_color(holds ? UI_GOOD : UI_BAD);
        vga_print_str(holds ? "  verdict  : DEFENCE HOLDS - nothing to refuse, selftest passes\n"
                            : "  verdict  : NOT BLOCKED - no refusal and the selftest failed\n");
    }
    ui_reset();
    vga_print_str("  run 'seclab repair' to clean up\n");
}

static void do_defence(const char *id) {
    // Accept either the defence id or the id of the attack it stops.
    const defence_t *d = proc_security_defence_by_id(id);
    if (!d) {
        const attack_t *a = proc_security_attack_by_id(id);
        if (a && a->defence)
            d = proc_security_defence_by_id(a->defence);
    }
    if (!d) {
        ui_start(UI_ERROR, "seclab");
        ui_printf("no defence matched '%s'", id);
        ui_end();
        return;
    }

    ui_color(UI_GOOD);
    vga_print_str("\nDEFENCE ");
    vga_print_str(d->id);
    ui_reset();
    vga_print_str("\n  fix      : ");
    vga_print_str(d->fix);
    vga_print_str("\n  stops    : ");
    vga_print_str(d->attack ? d->attack : "(none)");
    vga_print_str("\n");
}

static void do_verify(const char *id) {
    if (strcmp(id, "all") != 0) {
        const defence_t *d = proc_security_defence_by_id(id);
        if (d)
            print_selftest(d);
        else
            ui_error("seclab", "no defence with that id (try: seclab list)");
        return;
    }

    int total = proc_security_defence_count();
    int holding = 0;

    ui_color(UI_TITLE);
    vga_print_str("\nRunning every defence selftest\n");
    ui_reset();

    for (int i = 0; i < total; i++)
        holding += print_selftest(proc_security_defence(i));

    vga_print_str("\n  ");
    vga_print_int(holding);
    vga_print_str(" of ");
    vga_print_int(total);
    vga_print_str(" defences hold\n");
}

static void do_log(void) {
    int n = sec_log_count();

    ui_color(UI_TITLE);
    vga_print_str("\nSecurity events (newest first)\n");
    ui_reset();

    if (n == 0) {
        vga_print_str("  none yet - no defence has refused anything\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        const sec_event_t *e = sec_log_get(i);
        vga_print_str("  [tick ");
        vga_print_int((int)e->tick);
        vga_print_str("] ");
        ui_color(UI_BAD);
        vga_print_str(e->defence);
        ui_reset();
        vga_print_str(" - ");
        vga_print_str(e->detail);
        vga_print_str("\n");
    }
}

static void do_status(void) {
    ui_color(UI_TITLE);
    vga_print_str("\nSystem Security Lab status\n");
    ui_reset();

    vga_print_str("  attacks registered  : ");
    vga_print_int(proc_security_attack_count());
    vga_print_str("\n  defences registered : ");
    vga_print_int(proc_security_defence_count());
    vga_print_str("\n  refusals logged     : ");
    vga_print_int((int)sec_total_denied());
    vga_print_str("\n  watchdog            : ");
    if (sec_watchdog_fired()) {
        ui_color(UI_BAD);
        vga_print_str("FIRED on ");
        vga_print_str(sec_watchdog_attack() ? sec_watchdog_attack() : "?");
        ui_reset();
    } else {
        vga_print_str("idle");
    }

    // Free process slots: the resource most attacks go after.
    vga_print_str("\n  free process slots  : ");
    vga_print_int(process_count_free_slots());
    vga_print_str(" / ");
    vga_print_int(MAX_PROCESSES);
    vga_print_str("\n");
}

static void do_repair(void) {
    vga_print_str("\nAsking every attack to undo itself\n");

    for (int i = 0; i < proc_security_attack_count(); i++) {
        const attack_t *a = proc_security_attack(i);
        if (a->cleanup)
            a->cleanup();
    }

    def_reap_zombies();     // free what the cleanups just killed
    vga_print_str("  done\n");
}

static void print_usage(void) {
    ui_usage("seclab list | info <id> | attack <id> [live]");
    vga_print_str("       seclab defence <id> | verify <id|all>\n");
    vga_print_str("       seclab log | status | repair\n");
}

// Live attacks and repair start and kill processes of every user, so
// they are root's job. Everything else only reads and is open to all.
static int need_root(const char *what) {
    if (process_caller_uid() == UID_ROOT)
        return 1;
    ui_start(UI_ERROR, "seclab");
    ui_printf("%s needs root: type 'su' first", what);
    ui_end();
    return 0;
}

/* ---------- Entry point ---------- */

static void cmd_seclab(const char *args) {
    char sub[24], id[32], mode[16];

    const char *rest = arg_next(args, sub, sizeof(sub));
    rest = arg_next(rest, id, sizeof(id));
    arg_next(rest, mode, sizeof(mode));

    if (sub[0] == '\0' || strcmp(sub, "list") == 0) do_list();
    else if (strcmp(sub, "info") == 0)              do_info(id);
    else if (strcmp(sub, "attack") == 0) {
        if (strcmp(mode, "live") != 0 || need_root("a live attack"))
            do_attack(id, mode);
    }
    else if (strcmp(sub, "defence") == 0)           do_defence(id);
    else if (strcmp(sub, "verify") == 0)            do_verify(id);
    else if (strcmp(sub, "verify-all") == 0)        do_verify("all");
    else if (strcmp(sub, "log") == 0)               do_log();
    else if (strcmp(sub, "status") == 0)            do_status();
    else if (strcmp(sub, "repair") == 0) {
        if (need_root("repair"))
            do_repair();
    }
    else                                            print_usage();
}

void register_security_lab_commands(void) {
    shell_begin_group("Security lab");
    shell_register_command("seclab", "System security lab: attacks and defences", cmd_seclab);
}
