/*
 * atk_kernel_pointer_leak.c - ATTACK kernel_pointer_leak: learn where the
 * kernel keeps things.
 *
 * Breaks:     disclosure - kernel addresses help an exploit aim
 * Stopped by: kptr_hide (defence/kptr_hide)
 *
 * procinfo shows a process's kernel stack, its saved stack pointer and
 * the kernel function it started in. For an attacker that is a map:
 * where to aim an overflow, where to jump.
 *
 * simulate(): name the three addresses.
 * launch():   as user 1, ask for them about user 1's OWN process, the
 *             way procinfo does. findings = real addresses shown.
 * cleanup():  kill the process.
 *
 * Uses: process_create / process_kill / process_get_by_pid
 *       (core/process), def_kptr_show (its defence), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/kernel_pointer_leak/defence/kptr_hide/def_kptr_hide.h"  // def_kptr_show
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1

static int pid = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  kernel stack, saved ESP, entry function of a process\n");
    ui_printf("  shown to its owner: a map for an exploit\n");
    return 3;
}

static int launch(void) {
    pid = process_create("kptr_probe", sleeper, ATTACKER_UID);
    process_t *p = pid > 0 ? process_get_by_pid(pid) : 0;
    if (!p) {
        ui_printf("  could not start the process\n");
        return 0;
    }
    const struct { const char *what; uint32_t addr; } leaks[] = {
        { "kernel stack", p->stack_base },
        { "saved ESP",    p->esp },
        { "entry",        (uint32_t)p->entry },
    };
    int shown = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t seen = def_kptr_show(ATTACKER_UID, leaks[i].addr);
        shown += seen != 0;
        ui_printf("  %-14s real %x  user %d sees ", leaks[i].what, leaks[i].addr, ATTACKER_UID);
        ui_color(seen ? UI_BAD : UI_GOOD);
        ui_printf("%x\n", seen);
        ui_reset();
    }
    return shown;
}

static int cleanup(void) {
    int killed = pid > 0 && process_get_by_pid(pid) ? process_kill(pid) : 0;
    pid = -1;
    return killed;
}

static const attack_t attack = {
    "kernel_pointer_leak",
    "read kernel stack / ESP / entry addresses of a process",
    INV_DISCLOSURE, SEV_LOW,
    simulate, launch, cleanup,
    "kptr_hide"
};

void atk_kernel_pointer_leak_register(void) { registry_add_attack(&attack); }
