#ifndef AFFINITY_H
#define AFFINITY_H

/*
 * affinity.h - Which CPUs a process may run on.
 *
 * On a machine with many CPUs each process carries a bit mask:
 *
 *     mask 0b0101   = may run on CPU 0 and CPU 2, never on 1 or 3
 *
 * The scheduler only puts a process on a CPU whose bit is set. That is
 * "CPU affinity" (taskset, sched_setaffinity). It keeps a program near
 * its cache, or keeps busy work away from a CPU reserved for something
 * else.
 *
 * UzOS starts ONE CPU (it has no code to wake the others: no SMP). So
 * there is exactly one valid choice, CPU 0, and every mask must contain
 * it. The rules and the mask are still real, and would work unchanged
 * on more CPUs: only CPU_COUNT would grow.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define CPU_COUNT 1     // CPUs UzOS runs on: just the boot CPU

// Bit n set = CPU n exists and runs ("online"): 0b1.
uint32_t affinity_online_mask(void);

// How many CPUs are online (1).
int affinity_cpu_count(void);

// The CPU this code runs on right now (always 0).
int affinity_current_cpu(void);

// The CPUs 'p' may run on: its mask, limited to the online ones.
uint32_t affinity_get(const process_t *p);

// Set the mask of 'pid', asked by 'caller' (NULL = the kernel).
// Refused (0) if: no such process, a system process, someone else's
// process without CAP_PRIORITY, or no online CPU in the mask.
int affinity_set(const process_t *caller, uint32_t pid, uint32_t mask);

// "0", "0-3", "0,2": the CPU numbers in 'mask', as Linux writes them.
void affinity_list(uint32_t mask, char *out, int size);

// Read a CPU list ("0", "0-2", "0,1") into a mask. 0 if it is not one.
uint32_t affinity_parse_list(const char *text);

#endif
