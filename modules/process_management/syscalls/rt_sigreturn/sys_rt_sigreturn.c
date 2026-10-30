/*
 * sys_rt_sigreturn.c - rt_sigreturn(): end a signal handler.
 *
 * The handler's 'ret' took it to the frame's trampoline (or its
 * restorer), which runs `mov eax, 76 ; int 0x80`. ESP now points just
 * past frame->ret, so the frame is at ESP - 4. Every register saved in
 * it is put back into 'regs', so the iret lands exactly where the
 * program was when the signal came, with its old blocked mask.
 *
 * The frame is in the program's own memory: it may have been edited or
 * forged. sigframe_check decides what may come back; a refused frame
 * ends the program with SIGSEGV.
 *
 * Returns the restored EAX (the syscall handler writes it to regs->eax).
 *
 * Uses: scheduler_get_current_process (core/scheduler), process_exit_with
 *       (core/process), sigframe_t (core/signal), def_user_range_ok
 *       (memory_management defence user_ptr_bounds), def_sigframe_check
 *       (security/sigreturn_forgery/defence/sigframe_check), ui (a line
 *       saying why the program was ended).
 */
#include "modules/process_management/syscalls/rt_sigreturn/sys_rt_sigreturn.h"                                // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                              // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                                                    // sigframe_t, SIG_UNBLOCKABLE
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"                   // def_user_range_ok
#include "modules/process_management/security/sigreturn_forgery/defence/sigframe_check/def_sigframe_check.h"  // def_sigframe_check
#include "modules/ui/core/style/ui.h"                                                                         // ui_color, ui_printf

static void refuse(process_t *p) __attribute__((noreturn));
static void refuse(process_t *p) {
    uint8_t color = vga_get_color();
    ui_color(UI_DIM);
    ui_printf("[signal] PID %d (%s): bad rt_sigreturn frame, ended by SIGSEGV\n", p->pid, p->name);
    vga_set_color_byte(color);
    process_exit_with(SIGSEGV);
}

uint32_t sys_rt_sigreturn(registers_t *regs) {
    process_t *self = scheduler_get_current_process();
    if (!self || (regs->cs & 3) != 3)
        return (uint32_t)-1;                // only ring 3 programs get frames

    uint32_t frame = regs->useresp - 4;     // 'ret' was popped by the handler
    if (!def_user_range_ok(frame, sizeof(sigframe_t)))
        refuse(self);

    sigcontext_t ctx = ((sigframe_t *)frame)->saved;    // a copy: checked, then used

    // DEFENCE sigframe_check: only the newest frame the kernel pushed,
    // only user-space EIP/ESP, and EFLAGS without IOPL/NT/VM.
    if (!def_sigframe_check(self, frame, &ctx))
        refuse(self);

    regs->ebx = ctx.ebx;  regs->ecx = ctx.ecx;  regs->edx = ctx.edx;
    regs->esi = ctx.esi;  regs->edi = ctx.edi;  regs->ebp = ctx.ebp;
    regs->useresp = ctx.esp;
    regs->eip     = ctx.eip;
    regs->eflags  = ctx.eflags;
    self->sig_blocked = ctx.blocked & ~SIG_UNBLOCKABLE;
    return ctx.eax;
}
