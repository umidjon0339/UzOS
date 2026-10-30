/*
 * atk_handler_hijack.c - ATTACK handler_hijack: become unkillable, or aim
 * a handler at the kernel.
 *
 * Breaks:     integrity - SIGKILL must always work; the kernel decides
 *             where its own code runs
 * Stopped by: handler_bounds (defence/handler_bounds)
 *
 * rt_sigaction() tells the kernel "when signal N comes, jump to address
 * X". A program that sets a handler for SIGKILL, or SIG_IGN for SIGSTOP,
 * could no longer be stopped by anyone. A handler address inside the
 * kernel asks the kernel to jump into its own code at a moment the
 * program chooses.
 *
 * simulate(): list the requests.
 * launch():   ask handler_bounds about each, as rt_sigaction does.
 *             findings = requests that got through.
 * cleanup():  nothing (launch only asks the rule).
 *
 * Uses: def_handler_ok (its defence), USER_SPACE_START (memory layout), ui.
 */

#include "modules/security_management/core/registry/registry.h"                                            // attack_t, registry_add_attack
#include "modules/process_management/security/handler_hijack/defence/handler_bounds/def_handler_bounds.h"  // def_handler_ok
#include "modules/memory_management/core/layout/memory_layout.h"                                           // USER_SPACE_START
#include "modules/ui/core/style/ui.h"                                                                      // ui_printf, ui_color

static const struct {
    const char   *what;
    int           sig;
    k_sigaction_t act;
} requests[] = {
    { "catch SIGKILL (unkillable)",        SIGKILL, { USER_SPACE_START + 0x100, 0, 0, 0 } },
    { "ignore SIGSTOP (unstoppable)",      SIGSTOP, { SIG_IGN, 0, 0, 0 } },
    { "SIGUSR1 handler = kernel code",     SIGUSR1, { 0x00100000, 0, 0, 0 } },
    { "SIGUSR1 restorer = kernel heap",    SIGUSR1, { USER_SPACE_START + 0x100, SA_RESTORER, 0xC0001000, 0 } },
};
#define REQUEST_COUNT (sizeof(requests) / sizeof(requests[0]))

static int simulate(void) {
    for (unsigned i = 0; i < REQUEST_COUNT; i++)
        ui_printf("  rt_sigaction: %s\n", requests[i].what);
    return REQUEST_COUNT;
}

static int launch(void) {
    int through = 0;
    for (unsigned i = 0; i < REQUEST_COUNT; i++) {
        int ok = def_handler_ok(requests[i].sig, &requests[i].act);
        through += ok;
        ui_printf("  %-34s ", requests[i].what);
        ui_color(ok ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", ok ? "accepted!" : "refused");
        ui_reset();
    }
    return through;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "handler_hijack",
    "rt_sigaction: catch SIGKILL, ignore SIGSTOP, handler in the kernel",
    INV_INTEGRITY, SEV_HIGH,
    simulate, launch, cleanup,
    "handler_bounds"
};

void atk_handler_hijack_register(void) { registry_add_attack(&attack); }
