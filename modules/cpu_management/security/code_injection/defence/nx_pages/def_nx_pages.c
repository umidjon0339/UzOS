/*
 * def_nx_pages.c - DEFENCE nx_pages (CONCEPT): do not execute data.
 *
 * Stops:  code_injection (CONCEPT - see README)
 * Called: nothing: the no-execute bit is a CPU feature UzOS's 32-bit
 *         paging (no PAE) does not provide, so a page cannot be marked
 *         non-executable here.
 * Rule:   a writable data page (stack, heap, input buffer) must not be
 *         executable; the CPU faults if the program jumps into it. That
 *         turns injected bytes from "new code" into just a crash.
 * Why:    honesty - without an NX bit UzOS cannot enforce this. The
 *         lesson is the rule and why this kernel cannot apply it; see
 *         also wx_and_aslr (ROP), the no-injection cousin of this one.
 */

#include "modules/cpu_management/security/code_injection/defence/nx_pages/def_nx_pages.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/memory_management/core/vmm/vmm.h"

int def_data_pages_could_be_nx(void) {
    // UzOS marks user data pages PAGE_USER|PAGE_WRITABLE but has no bit
    // to also say "not executable", so this stays a concept.
    return (PAGE_USER != 0) && (PAGE_WRITABLE != 0);
}

static int selftest(void) { return def_data_pages_could_be_nx(); }

static const defence_t defence = {
    "nx_pages",
    "concept: NX data pages stop injected code; UzOS paging has no NX bit",
    "code_injection",
    selftest
};
void def_nx_pages_register(void) { registry_add_defence(&defence); }
