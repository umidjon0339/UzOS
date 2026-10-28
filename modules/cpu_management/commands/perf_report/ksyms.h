#ifndef KSYMS_H
#define KSYMS_H

/*
 * ksyms.h - The kernel's own function names ("kallsyms" in Linux).
 *
 * The CPU only knows addresses: a sample says "EIP was 0x00104a2c". To
 * tell a person WHICH function that is, the kernel carries a table of
 * every function's start address and name, sorted by address:
 *
 *     0x00104a00  idle_task
 *     0x00104a40  scheduler_init      <- 0x00104a2c is inside idle_task
 *
 * The table is made at build time (ksyms_gen.sh, see the Makefile) and
 * linked into the kernel. Only `perf report` uses it, so it lives in
 * its folder (commands/perf_report/).
 */

#include <stdint.h>

typedef struct {
    uint32_t    address;    // where the function starts
    const char *name;       // its name in the source code
} ksym_t;

// The function that contains 'address', or NULL if none does (an
// address outside the kernel's code, e.g. a ring 3 program).
// *offset (if not NULL) = how far into the function the address is.
const char *ksyms_lookup(uint32_t address, uint32_t *offset);

// How many functions the table holds (0 if the kernel was linked without it).
uint32_t ksyms_count(void);

#endif
