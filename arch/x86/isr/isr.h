#ifndef ISR_H
#define ISR_H

/*
 * isr.h - Interrupt Service Routines.
 *
 * Every interrupt enters an assembly stub (isr_stubs.s) which saves the
 * CPU registers and calls isr_handler() in C. Drivers hook a vector with
 * register_interrupt_handler(), e.g. the timer uses vector 32.
 */

#include <stdint.h>

#define ISR_EXCEPTIONS 32       // vectors 0-31
#define ISR_IRQS       16       // IRQ 0-15
#define IRQ_BASE       32       // IRQ 0 = vector 32 (set by pic_remap)
#define SYSCALL_VECTOR 128      // int 0x80

// Saved CPU state, in the exact order the stub pushes it (lowest first).
typedef struct registers {
    uint32_t ds;                                     // pushed by the stub
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushed by 'pusha'
    uint32_t int_no, err_code;                       // pushed by the stub (or CPU)
    uint32_t eip, cs, eflags;                        // pushed by the CPU
    uint32_t useresp, ss;                            // only if coming from ring 3
} registers_t;

// A C interrupt handler. It may change *regs (e.g. eax = syscall result).
typedef void (*isr_t)(registers_t *regs);

// Run 'handler' whenever vector 'n' fires.
void register_interrupt_handler(uint8_t n, isr_t handler);

// Does vector 'n' have a handler?
int isr_has_handler(uint8_t n);

// How many times IRQ line 'irq' (0-15) has fired, and how many
// spurious IRQs were ignored. Shown by the `devices` command.
uint32_t isr_irq_count(uint8_t irq);
uint32_t isr_spurious_count(void);

// The name of CPU exception 'n' (0-31): "Division By Zero", ...
const char *isr_exception_name(uint8_t n);

// An exception the kernel cannot survive: print the registers and halt.
// Used for faults with no handler, and by handlers that give up
// (cpu_management/core/faults decides which faults those are).
void isr_panic(registers_t *regs) __attribute__((noreturn));

// Called by the assembly stub for every interrupt.
void isr_handler(registers_t *regs);

// Stub addresses, used by idt.c to fill the IDT.
extern uint32_t isr_stub_table[ISR_EXCEPTIONS];
extern uint32_t irq_stub_table[ISR_IRQS];
extern void isr128(void);

#endif
