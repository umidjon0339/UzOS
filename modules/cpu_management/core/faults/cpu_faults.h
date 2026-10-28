#ifndef CPU_FAULTS_H
#define CPU_FAULTS_H

/*
 * cpu_faults.h - What happens when the CPU refuses to run an instruction.
 *
 * Some instructions cannot finish: divide by zero, an opcode the CPU does
 * not know, `cli` from ring 3, ... The CPU then stops and raises an
 * EXCEPTION (vectors 0-31). Someone must decide what that means:
 *
 *   the fault came from ring 3   the program has a bug or is hostile:
 *                                kill that one process, the OS runs on
 *   the fault came from ring 0   the KERNEL has a bug: nothing can be
 *                                trusted any more, so stop (isr_panic)
 *
 * That rule is the defence fault_isolation (security/user_exception/defence/). This
 * file registers a handler for every exception and applies it. Page
 * faults (14) keep their own handler in memory_management/core/vmm,
 * which uses the same rule and the same record.
 */

#include <stdint.h>
#include "arch/x86/isr/isr.h"
#include "modules/process_management/core/process/process.h"

#define CPU_FAULT_LOG_SIZE 8        // the newest faults `faults` shows

// One process killed by a CPU exception.
typedef struct {
    uint32_t pid;
    char     name[PROCESS_NAME_LEN];
    uint8_t  vector;                // which exception (0-31)
    uint32_t eip;                   // the instruction that faulted
    const char *instruction;        // "cli", "div", ... or 0 if unknown
    uint32_t tick;                  // when (timer ticks since boot)
} cpu_fault_t;

// Hook every CPU exception except the page fault. Called from kernel_main
// right after the IDT is set up.
void cpu_faults_init(void);

// Print what went wrong ("[CPU FAULT] ...") and remember it. The caller
// then ends the process with process_exit().
void cpu_fault_record(const registers_t *regs);

// One line explaining exception 'vector' in plain words.
const char *cpu_fault_hint(uint8_t vector);

// Processes killed by exception 'vector' since boot, and by all of them.
uint32_t cpu_fault_kills(uint8_t vector);
uint32_t cpu_fault_total(void);

// The newest faults: index 0 = the latest one.
int cpu_fault_log_count(void);
const cpu_fault_t *cpu_fault_log_get(int index);

#endif
