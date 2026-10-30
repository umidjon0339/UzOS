/*
 * sys_ptrace.c - ptrace(request, pid, addr, data): a debugger's syscall.
 *
 *   ATTACH pid    become its tracer (ptrace_restrict); it gets SIGSTOP
 *   TRACEME       my parent becomes my tracer (ptrace_restrict, the
 *                 other way round)
 *   PEEKDATA      read a word of its memory into *data (vm_access_check)
 *   POKEDATA      write 'data' into a word of its memory (vm_access_check)
 *   GETREGS       its ring 3 registers, as saved when it entered the
 *                 kernel (the top of its kernel stack), into *data
 *   CONT / KILL   let it run on / end it
 *   DETACH        stop tracing; it carries on
 *
 * Every request but ATTACH and TRACEME needs the caller to BE the
 * tracee's tracer; PEEK, POKE and GETREGS also need it stopped, as in
 * Linux. When a tracer ends, its tracees are let go (process_ended).
 * Returns 0 (or the data asked for), -1 if refused.
 *
 * Uses: scheduler_get_current_process / process_get_by_pid / process_kill
 *       (core/process), signal_send (core/signal), registers_t
 *       (arch/x86/isr), def_ptrace_attach_ok (security/ptrace_abuse/
 *       defence/ptrace_restrict), def_vm_access_ok (security/memory_peek/
 *       defence/vm_access_check).
 */
#include "modules/process_management/syscalls/ptrace/sys_ptrace.h"     // this file
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"             // signal_send, SIGSTOP, SIGCONT
#include "arch/x86/isr/isr.h"                                          // registers_t
#include "modules/process_management/security/ptrace_abuse/defence/ptrace_restrict/def_ptrace_restrict.h"  // def_ptrace_attach_ok
#include "modules/process_management/security/memory_peek/defence/vm_access_check/def_vm_access_check.h"   // def_vm_access_ok

// The registers 't' saved when it last came from ring 3: the top of its
// kernel stack (the TSS points ring 3 interrupts there). NULL if it is
// not a ring 3 program caught inside the kernel.
static registers_t *saved_user_regs(const process_t *t) {
    if (!t->stack_base)
        return 0;
    registers_t *r = (registers_t *)(t->stack_base + PROCESS_STACK_SIZE) - 1;
    return (r->cs & 3) == 3 ? r : 0;
}

int sys_ptrace(int request, uint32_t pid, uint32_t addr, uint32_t data) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;

    if (request == PTRACE_TRACEME) {
        process_t *parent = process_get_by_pid(self->ppid);
        // DEFENCE ptrace_restrict: the parent must be allowed to attach to us.
        if (!parent || !def_ptrace_attach_ok(parent, self))
            return -1;
        self->tracer = parent->pid;
        return 0;
    }

    process_t *t = process_get_by_pid(pid);
    if (!t || t->state == PROCESS_STATE_ZOMBIE)
        return -1;                                      // ESRCH

    if (request == PTRACE_ATTACH) {
        // DEFENCE ptrace_restrict: own, dumpable, untraced, a descendant.
        if (!def_ptrace_attach_ok(self, t))
            return -1;
        t->tracer = self->pid;
        signal_send(t, SIGSTOP);
        return 0;
    }

    if (t->tracer != self->pid)
        return -1;                                      // ESRCH: not our tracee
    int stopped = t->state == PROCESS_STATE_STOPPED;

    switch (request) {
        case PTRACE_PEEKDATA:
            // DEFENCE vm_access_check: its user memory, with rights.
            if (!stopped || !def_vm_access_ok(self, t, addr, 4))
                return -1;
            *(uint32_t *)data = *(const uint32_t *)addr;   // one address space
            return 0;
        case PTRACE_POKEDATA:
            if (!stopped || !def_vm_access_ok(self, t, addr, 4))
                return -1;
            *(uint32_t *)addr = data;
            return 0;
        case PTRACE_GETREGS: {
            registers_t *r = saved_user_regs(t);
            if (!stopped || !r)
                return -1;
            ptrace_regs_t *out = (ptrace_regs_t *)data;
            out->ebx = r->ebx;  out->ecx = r->ecx;  out->edx = r->edx;
            out->esi = r->esi;  out->edi = r->edi;  out->ebp = r->ebp;
            out->eax = r->eax;  out->orig_eax = r->eax;
            out->ds = out->es = out->fs = out->gs = r->ds;
            out->eip = r->eip;  out->cs = r->cs;    out->eflags = r->eflags;
            out->esp = r->useresp;  out->ss = r->ss;
            return 0;
        }
        case PTRACE_CONT:
            signal_send(t, SIGCONT);
            return 0;
        case PTRACE_KILL:
            return process_kill(t->pid) ? 0 : -1;
        case PTRACE_DETACH:
            t->tracer = 0;
            signal_send(t, SIGCONT);
            return 0;
        default:
            return -1;                                  // EIO: not in UzOS
    }
}
