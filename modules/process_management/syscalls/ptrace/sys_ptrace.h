#ifndef SYS_PTRACE_H
#define SYS_PTRACE_H
/* sys_ptrace.h - ptrace(request, pid, addr, data): debug another process. */
#include <stdint.h>
#define SYS_PTRACE 93           // EAX=93, EBX=request, ECX=pid, EDX=addr, ESI=data

// Requests (the same numbers as Linux).
#define PTRACE_TRACEME   0      // my parent becomes my tracer
#define PTRACE_PEEKDATA  2      // *data = the word at addr in the tracee
#define PTRACE_POKEDATA  5      // the word at addr in the tracee = data
#define PTRACE_CONT      7      // let the stopped tracee run
#define PTRACE_KILL      8      // end the tracee
#define PTRACE_GETREGS  12      // *data = its registers (ptrace_regs_t)
#define PTRACE_ATTACH   16      // become its tracer; it stops (SIGSTOP)
#define PTRACE_DETACH   17      // stop tracing; it carries on

// struct user_regs_struct of 32-bit Linux: what GETREGS fills.
typedef struct {
    uint32_t ebx, ecx, edx, esi, edi, ebp, eax;
    uint32_t ds, es, fs, gs, orig_eax;
    uint32_t eip, cs, eflags, esp, ss;
} ptrace_regs_t;

int sys_ptrace(int request, uint32_t pid, uint32_t addr, uint32_t data);
#endif
