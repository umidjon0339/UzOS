/*
 * def_pcb_wipe.c - DEFENCE pcb_wipe: a reused slot starts empty.
 *
 * Stops:  uninit_pcb (security/attack/uninit_pcb)
 * Called: process_init() for every slot, process_create() for the new one.
 * Rule:   zero the whole process_t before anyone writes to it.
 * Why:    slots are reused many times. A field the new owner does not
 *         set (cwd, open files, ...) would still hold the old owner's
 *         value: an information leak, and a stale pointer waiting to
 *         crash. "Every field is either mine or zero" is easy to trust.
 */

#include "def_pcb_wipe.h"
#include "registry.h"
#include "string.h"

void def_pcb_wipe(process_t *p) {
    if (!p)
        return;
    memset(p, 0, sizeof(*p));
    p->state = PROCESS_STATE_FREE;
}

static int selftest(void) {
    process_t probe;

    // Pretend a previous process left its data in the slot.
    memset(&probe, 0xAB, sizeof(probe));

    def_pcb_wipe(&probe);

    const uint8_t *bytes = (const uint8_t *)&probe;
    for (unsigned i = 0; i < sizeof(probe); i++)
        if (bytes[i] != 0)
            return 0;               // something survived the wipe
    return probe.state == PROCESS_STATE_FREE;
}

static const defence_t defence = {
    "pcb_wipe",
    "zero a process slot before giving it to a new process",
    "uninit_pcb",
    selftest
};

void def_pcb_wipe_register(void) {
    registry_add_defence(&defence);
}
