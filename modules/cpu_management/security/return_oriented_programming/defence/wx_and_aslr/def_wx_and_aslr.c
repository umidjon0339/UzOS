/*
 * def_wx_and_aslr.c - DEFENCE wx_and_aslr (CONCEPT): make code reuse hard.
 *
 * Stops:  return_oriented_programming (CONCEPT - see README)
 * Called: nothing new: W^X and the no-execute bit are CPU features UzOS
 *         does not have (32-bit paging without PAE has no NX bit), and
 *         ASLR/CFI are build-time choices. This reports the honest gap.
 * Rule:   W^X: no page is both writable and executable. ASLR: gadget
 *         addresses are unpredictable. CFI: a return must go to a real
 *         call site. Together they make a ROP chain hard to build.
 * Why:    honesty - UzOS's page model has PAGE_WRITABLE and PAGE_USER
 *         but no execute bit, so it cannot enforce W^X. The lesson is
 *         what the defences are and why this kernel cannot run them.
 */

#include "modules/cpu_management/security/return_oriented_programming/defence/wx_and_aslr/def_wx_and_aslr.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/memory_management/core/vmm/vmm.h"   // PAGE_WRITABLE, PAGE_USER

int def_page_bits_present(void) {
    return (PAGE_WRITABLE != 0) && (PAGE_USER != 0);   // the bits we do have
}

static int selftest(void) {
    // UzOS has the write/user bits; it does NOT have a no-execute bit,
    // so this defence is a concept, not an enforced rule.
    return def_page_bits_present();
}

static const defence_t defence = {
    "wx_and_aslr",
    "concept: W^X/ASLR/CFI stop ROP; UzOS 32-bit paging has no NX bit",
    "return_oriented_programming",
    selftest
};
void def_wx_and_aslr_register(void) { registry_add_defence(&defence); }
