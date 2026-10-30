/*
 * deliver.c - Act on a signal on the way back to ring 3.
 *
 * Runs at the end of every interrupt or syscall that came from ring 3
 * (isr_on_return_to_user), so only ring 3 programs get here. It takes
 * the first pending signal that is not blocked, and:
 *
 *   SIG_IGN               nothing
 *   SIG_DFL               the default action: ignore, stop, or end the
 *                         process (exit status = the signal)
 *   a handler             build a sigframe_t on the program's stack, and
 *                         make the iret land in the handler:
 *
 *       program's stack (grows down)        registers after this
 *       +----------------------------+
 *       | code: mov eax,76 ; int 0x80|      EIP = handler
 *       | saved: every register, the |      ESP = &frame->ret
 *       |        mask before         |
 *       | siginfo                    |
 *       | context, info, signo       |  <- the handler's arguments
 *       | ret -> restorer or 'code'  |  <- ESP
 *       +----------------------------+
 *
 *   When the handler returns, 'ret' takes it to rt_sigreturn, which puts
 *   every saved register back (sigframe_check guards that step).
 *
 * The frame must fit in the program's own mapped memory
 * (user_ptr_bounds); if it does not, the program is ended with SIGSEGV,
 * as Linux does.
 *
 * Uses: the current process (core/scheduler), process_exit_with
 *       (core/process), sigqueue_take (sigqueue.c), def_user_range_ok
 *       (memory_management defence user_ptr_bounds), ui_* (ui) to say
 *       which signal ended a process.
 */

#include "modules/process_management/core/signal/signal.h"                                   // this part
#include "modules/process_management/core/scheduler/scheduler.h"                             // scheduler_get_current_process, scheduler_yield
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"  // def_user_range_ok
#include "modules/ui/core/style/ui.h"                                                        // ui_color, ui_printf
#include "lib/string/string.h"                                                               // memset

#define EFLAGS_TF 0x100         // trap flag: single step
#define EFLAGS_DF 0x400         // direction flag: C code expects it clear
#define CORE_DUMP 0x80          // exit status bit: "would have dumped core"

static int lowest_bit(uint32_t mask) {
    for (int sig = 1; sig < SIGNAL_COUNT; sig++)
        if (mask & SIG_BIT(sig))
            return sig;
    return 0;
}

static int on_altstack(const process_t *p, uint32_t sp) {
    return p->altstack_size && sp >= p->altstack_sp &&
           sp < p->altstack_sp + p->altstack_size;
}

static void announce(const process_t *p, int sig) {
    uint8_t color = vga_get_color();
    ui_color(UI_DIM);
    ui_printf("[signal] PID %d (%s) ended by SIG%s\n", p->pid, p->name, signal_name(sig));
    vga_set_color_byte(color);
}

// Put a handler frame on the program's stack. 0 = it does not fit.
static int push_frame(process_t *p, registers_t *regs, int sig, const k_siginfo_t *info) {
    uint32_t flags = p->sig_flags[sig];
    uint32_t sp = regs->useresp;

    if (p->sig_frames >= SIGNAL_MAX_FRAMES)
        return 0;                                   // handlers nested too deep
    if ((flags & SA_ONSTACK) && p->altstack_size && !(p->altstack_flags & SS_DISABLE) &&
        !on_altstack(p, sp))
        sp = p->altstack_sp + p->altstack_size;     // switch to the alternate stack

    sp = (sp - sizeof(sigframe_t)) & ~15u;          // 16-byte aligned, as the ABI wants
    if (!def_user_range_ok(sp, sizeof(sigframe_t)))
        return 0;

    // One address space: the program's stack is writable from here.
    sigframe_t *f = (sigframe_t *)sp;
    memset(f, 0, sizeof(*f));
    f->signo   = sig;
    f->info    = (uint32_t)&f->siginfo;
    f->context = (uint32_t)&f->saved;
    f->siginfo = *info;

    f->saved.eax = regs->eax;  f->saved.ebx = regs->ebx;
    f->saved.ecx = regs->ecx;  f->saved.edx = regs->edx;
    f->saved.esi = regs->esi;  f->saved.edi = regs->edi;
    f->saved.ebp = regs->ebp;  f->saved.esp = regs->useresp;
    f->saved.eip = regs->eip;  f->saved.eflags = regs->eflags;
    f->saved.blocked = p->sig_restore_mask ? p->sig_saved_mask : p->sig_blocked;
    p->sig_restore_mask = 0;            // rt_sigsuspend's mask comes back via the frame

    // mov eax, SYS_RT_SIGRETURN ; int 0x80   (used when there is no restorer)
    f->code[0] = 0xB8;
    f->code[1] = SYS_RT_SIGRETURN_NR;
    f->code[5] = 0xCD;
    f->code[6] = 0x80;
    f->ret = (flags & SA_RESTORER) ? p->sig_restorer[sig] : (uint32_t)f->code;

    // The frame is now "outstanding": rt_sigreturn may take it back once.
    p->sig_frame[p->sig_frames++] = sp;

    regs->useresp = sp;
    regs->eip     = p->sig_handler[sig];
    regs->eax     = sig;
    regs->eflags &= ~(EFLAGS_TF | EFLAGS_DF);

    p->sig_blocked |= p->sig_mask[sig];
    if (!(flags & SA_NODEFER))
        p->sig_blocked |= SIG_BIT(sig);
    p->sig_blocked &= ~SIG_UNBLOCKABLE;
    if (flags & SA_RESETHAND) {
        p->sig_handler[sig] = SIG_DFL;
        p->sig_flags[sig] &= ~SA_SIGINFO;
    }
    return 1;
}

void signal_deliver(registers_t *regs) {
    process_t *p = scheduler_get_current_process();
    if (!p || p->is_system)
        return;

    int sig;
    while ((sig = lowest_bit(signal_deliverable(p))) != 0) {
        p->signals &= ~SIG_BIT(sig);

        k_siginfo_t info;
        if (!sigqueue_take(p, sig, &info)) {        // a plain kill(): no value
            memset(&info, 0, sizeof(info));
            info.signo = sig;
            info.code  = p->sig_code[sig];
            info.pid   = p->sig_from[sig];
            process_t *from = info.pid ? process_get_by_pid(info.pid) : 0;
            info.uid   = from ? from->uid : 0;
        }
        if (sigqueue_waiting(p, sig))               // more of this one queued:
            p->signals |= SIG_BIT(sig);             // the next return delivers it

        uint32_t handler = sig == SIGKILL ? SIG_DFL : p->sig_handler[sig];
        if (handler == SIG_IGN)
            continue;

        if (handler == SIG_DFL) {
            switch (signal_default_action(sig)) {
                case ACT_IGNORE:
                case ACT_CONT:
                    continue;
                case ACT_STOP:
                    p->state = PROCESS_STATE_STOPPED;
                    scheduler_yield();              // back here after SIGCONT
                    continue;
                case ACT_CORE:
                    announce(p, sig);
                    process_exit_with(sig | CORE_DUMP);
                default:
                    announce(p, sig);
                    process_exit_with(sig);
            }
        }

        if (!push_frame(p, regs, sig, &info)) {     // no room for the frame
            announce(p, SIGSEGV);
            process_exit_with(SIGSEGV);
        }
        break;                  // one handler at a time; the next on its return
    }

    // rt_sigsuspend's mask, if no handler took it back with its frame.
    if (p->sig_restore_mask) {
        p->sig_blocked = p->sig_saved_mask;
        p->sig_restore_mask = 0;
    }
}
