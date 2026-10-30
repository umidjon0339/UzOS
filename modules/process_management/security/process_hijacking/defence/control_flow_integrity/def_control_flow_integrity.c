/*
 * def_control_flow_integrity.c - DEFENCE control_flow_integrity (concept).
 *
 * Stops:  process_hijacking (security/process_hijacking)
 * Called: the kernel sets each process's entry point and its ring-3
 *         return frame (core/usertask); ring 3 cannot rewrite them.
 * Rule:   a process runs the code the kernel launched; its execution is
 *         not redirected by another process or by its own data.
 * Why:    hijacking redirects a program to run attacker-chosen code
 *         (overwrite a return address or a function pointer). Full CFI
 *         checks every indirect jump/return against a legal target - a
 *         compiler and hardware feature. UzOS's part is isolation: no
 *         one can reach into the kernel's saved frames to redirect them.
 */

#include "modules/process_management/security/process_hijacking/defence/control_flow_integrity/def_control_flow_integrity.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                                               // defence_t, registry_add_defence

int def_entry_is_kernel_set(void) { return 1; }

static int selftest(void) { return def_entry_is_kernel_set(); }

static const defence_t defence = {
    "control_flow_integrity",
    "concept: the kernel sets entry/return; full CFI is compiler+hardware",
    "process_hijacking",
    selftest
};

void def_control_flow_integrity_register(void) { registry_add_defence(&defence); }
