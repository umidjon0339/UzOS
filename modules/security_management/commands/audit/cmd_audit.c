/*
 * cmd_audit.c - `audit [sum]`: what the defences refused, and to whom.
 *
 *   audit        the newest events, newest first:
 *
 *                   41.27s  guest   PID 2     kill_guard
 *                                             kill refused: not yours
 *
 *   audit sum    how often each defence refused since boot, and how
 *                many of those audit_limit only counted (a flood). These
 *                counters never forget, even when the ring is full.
 *
 * Uses: audit_snapshot / audit_source (core/audit), users_find_uid
 *       (user_management), timer_frequency (device_management),
 *       arg_next (lib/args), ui_* / ui_cell (ui module).
 */

#include "security_commands.h"
#include "audit.h"
#include "users.h"
#include "timer.h"
#include "string.h"
#include "format.h"
#include "args.h"
#include "ui.h"
#include "box.h"

// The detail goes on its own line, under the defence name:
// time (11 columns) + user (8) + PID (10) = 29.
#define DETAIL_INDENT "                             "

static audit_event_t events[AUDIT_SIZE];    // the snapshot we print

// "  41.27s" - ticks since boot as seconds.
static void print_time(uint32_t tick) {
    uint32_t hz = timer_frequency() ? timer_frequency() : TIMER_HZ;
    ui_printf("%5u.%02us  ", tick / hz, tick % hz * 100 / hz);
}

// A user's name, or "#1000" when the account was deleted since.
static void print_user(uint32_t uid) {
    const user_t *user = users_find_uid(uid);
    char number[12];

    ui_color(uid == UID_ROOT ? UI_BAD : UI_GOOD);
    if (user) {
        ui_cell(user->name, 8);
    } else {
        format(number, sizeof(number), "#%u", uid);
        ui_cell(number, 8);
    }
    ui_reset();
}

static void show_events(void) {
    int n = audit_snapshot(events);

    ui_title("Audit log (newest first)");
    ui_color(UI_DIM);
    ui_printf("  %d kept, %u refusals since boot, %u overwritten, %u only counted\n",
              n, audit_total(), audit_lost(), audit_dropped());
    ui_reset();

    if (n == 0) {
        vga_print_str("  none yet - no defence has refused anything\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        const audit_event_t *e = &events[i];

        print_time(e->tick);
        if (e->pid) {
            print_user(e->uid);
            ui_printf("PID %-4u  ", e->pid);
        } else {
            ui_cell("boot", 8);             // before the first process
            vga_print_str("          ");
        }
        ui_color(UI_BAD);
        vga_print_str(e->defence);
        ui_color(UI_DIM);
        vga_print_str("\n" DETAIL_INDENT);
        vga_print_str(e->detail);
        ui_end();
    }
}

static void show_sums(void) {
    ui_title("Refusals per defence since boot");
    ui_color(UI_TITLE);
    vga_print_str("  DEFENCE                 REFUSED  ONLY COUNTED\n");
    ui_reset();

    for (int i = 0; i < audit_source_count(); i++) {
        const audit_source_t *s = audit_source(i);
        vga_print_str("  ");
        ui_cell(s->defence, 22);
        ui_printf("%8u", s->refused);
        if (s->dropped)
            ui_printf("  %12u", s->dropped);
        vga_print_str("\n");
    }

    ui_color(UI_LABEL);
    vga_print_str("  ");
    ui_cell("total", 22);
    ui_printf("%8u  %12u", audit_total(), audit_dropped());
    ui_end();
}

void cmd_audit(const char *args) {
    char sub[8];
    arg_next(args, sub, sizeof(sub));

    if (sub[0] == '\0')
        show_events();
    else if (strcmp(sub, "sum") == 0)
        show_sums();
    else
        ui_usage("audit [sum]");
}
