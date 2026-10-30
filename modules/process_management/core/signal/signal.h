#ifndef SIGNAL_H
#define SIGNAL_H

/*
 * signal.h - Tell a process that something happened.
 *
 * A signal is a small message to a process: one bit in process_t.signals.
 * What happens next depends on who the process is:
 *
 *   ring 3 program   on its way back to ring 3 (after a syscall or an
 *                    interrupt) the kernel DELIVERS it (deliver.c):
 *                    runs its handler, or the default action
 *                    (terminate, stop, ignore)
 *   ring 0 process   it looks at its signals when it chooses
 *                    (the shell's commands: signal_interrupted())
 *   everyone         SIGSTOP / SIGCONT freeze and thaw it at once,
 *                    SIGKILL goes through process_kill()
 *
 *   Ctrl+C  -> the keyboard interrupt -> the shell -> signal_send(job, SIGINT)
 *   kill(pid, SIGUSR1) -> bit 10 set -> the program returns to ring 3
 *           -> its handler runs on its own stack -> rt_sigreturn -> back
 *
 * Two kinds of sleep (as in Linux):
 *   sys_sleep()                 uninterruptible: always sleeps the full time
 *   sys_sleep_interruptible()   a signal that is not blocked wakes it at once
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"  // process_t, SIGNAL_COUNT
#include "arch/x86/isr/isr.h"                                 // registers_t

/* ---------- The signals (the same numbers as Linux on x86) ---------- */

#define SIGHUP     1        // the controlling terminal went away
#define SIGINT     2        // "interrupt", sent by Ctrl+C
#define SIGQUIT    3
#define SIGILL     4        // illegal instruction
#define SIGTRAP    5
#define SIGABRT    6
#define SIGBUS     7
#define SIGFPE     8        // divide by zero
#define SIGKILL    9        // forced termination: cannot be caught, blocked or ignored
#define SIGUSR1   10        // free for programs to use
#define SIGSEGV   11        // bad memory access
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14        // alarm() went off
#define SIGTERM   15        // polite request to terminate (may be caught)
#define SIGSTKFLT 16
#define SIGCHLD   17        // a child ended (ignored unless handled)
#define SIGCONT   18        // carry on after a stop
#define SIGSTOP   19        // freeze: cannot be caught, blocked or ignored
#define SIGTSTP   20        // Ctrl+Z
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24        // RLIMIT_CPU used up
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28        // the window changed size
#define SIGIO     29
#define SIGPWR    30
#define SIGSYS    31        // bad system call (seccomp)

#define SIG_BIT(sig)    (1u << (sig))
#define SIG_UNBLOCKABLE (SIG_BIT(SIGKILL) | SIG_BIT(SIGSTOP))

/* ---------- What a process wants done (rt_sigaction) ---------- */

#define SIG_DFL 0u          // the default action
#define SIG_IGN 1u          // throw it away

#define SA_SIGINFO   0x00000004u    // handler(sig, siginfo *, ucontext *)
#define SA_RESTORER  0x04000000u    // return through sa_restorer
#define SA_ONSTACK   0x08000000u    // run on the sigaltstack
#define SA_RESTART   0x10000000u    // (accepted; UzOS syscalls do not restart)
#define SA_NODEFER   0x40000000u    // do not block the signal inside its handler
#define SA_RESETHAND 0x80000000u    // back to SIG_DFL after one delivery

// The kernel's struct sigaction on 32-bit x86. UzOS has 31 signals, so
// sa_mask is one 32-bit word.
typedef struct {
    uint32_t handler;       // SIG_DFL, SIG_IGN or a function in the program
    uint32_t flags;         // SA_*
    uint32_t restorer;      // with SA_RESTORER: the handler returns here
    uint32_t mask;          // also blocked while the handler runs
} k_sigaction_t;

// What happens to a signal nobody handles.
typedef enum { ACT_TERM, ACT_CORE, ACT_IGNORE, ACT_STOP, ACT_CONT } sig_action_t;

/* ---------- Where a handler may run (sigaltstack) ---------- */

#define SS_ONSTACK  1       // (old_ss only) running on it right now
#define SS_DISABLE  2       // no alternate stack
#define MINSIGSTKSZ 256     // a UzOS signal frame needs about 120 bytes

typedef struct {
    uint32_t sp;            // lowest address
    uint32_t flags;         // SS_*
    uint32_t size;          // bytes
} k_stack_t;

/* ---------- Who sent it, and why (siginfo) ---------- */

#define SI_USER    0        // kill(), tkill(), ...
#define SI_KERNEL  0x80     // the kernel (Ctrl+C, alarm)
#define SI_QUEUE   (-1)     // rt_sigqueueinfo(): carries a value
#define SI_TIMER   (-2)     // a timer (setitimer)

// UzOS's short siginfo_t: the first fields of Linux's, in the same order.
typedef struct {
    int32_t  signo;
    int32_t  error;         // si_errno
    int32_t  code;          // SI_*
    uint32_t pid;           // the sender
    uint32_t uid;           // the sender's user
    uint32_t value;         // si_value (rt_sigqueueinfo)
} k_siginfo_t;

/* ---------- The frame a handler runs on (deliver.c, rt_sigreturn) ---------- */

// Everything the handler's return must put back.
typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t esp, eip, eflags;
    uint32_t blocked;       // the mask before the handler
} sigcontext_t;

// Pushed on the program's stack (lowest address first). When the handler
// starts, ESP points at 'ret', so it sees its arguments as usual:
// handler(signo) or handler(signo, info, context).
typedef struct {
    uint32_t     ret;       // where 'ret' in the handler goes: the restorer,
                            // or 'code' below
    uint32_t     signo;     // 1st argument
    uint32_t     info;      // 2nd: &frame->siginfo
    uint32_t     context;   // 3rd: &frame->saved
    k_siginfo_t  siginfo;
    sigcontext_t saved;
    uint8_t      code[8];   // mov eax, SYS_RT_SIGRETURN ; int 0x80
} sigframe_t;

#define SYS_RT_SIGRETURN_NR 76      // the same as syscalls/rt_sigreturn

// Ring 3 programs build and read frames by hand (proccalls/signals.s,
// security/sigreturn_forgery/sigreturn_forgery.s): the layout is fixed.
_Static_assert(sizeof(sigframe_t) == 92, "the .s programs use this frame layout");
_Static_assert(sizeof(k_siginfo_t) == 24, "the .s programs use this siginfo layout");

/* ---------- Sending and looking ---------- */

// Hook signal delivery into every return to ring 3. Called by scheduler_init().
void signal_init(void);

// Send 'sig' to 'p' (no permission check: callers ask signal_permission).
// SIGSTOP / SIGCONT act at once; an ignored signal is thrown away; any
// other is marked pending, and an interruptible sleep ends if it is not
// blocked. Safe to call from an interrupt handler.
void signal_send(process_t *p, int sig);

// The same, with who sent it and why (code SI_*, value for SI_QUEUE).
void signal_send_info(process_t *p, int sig, int code, uint32_t value);

int  signal_pending(const process_t *p, int sig);   // is 'sig' pending on 'p'?
void signal_clear(process_t *p, int sig);           // forget 'sig'
uint32_t signal_deliverable(const process_t *p);    // pending and not blocked

// Has the running process been sent SIGINT? (For loops that should stop
// on Ctrl+C: `while (... && !signal_interrupted())`.)
int signal_interrupted(void);

// "SIGTERM" -> 15, "TERM" -> 15, "15" -> 15; 0 if unknown.
int signal_number(const char *name);
// 15 -> "TERM"; "?" if unknown.
const char *signal_name(int sig);
sig_action_t signal_default_action(int sig);

/* ---------- Interval timers (setitimer, alarm) ---------- */

#define ITIMER_REAL    0    // wall-clock time         -> SIGALRM (alarm() uses it too)
#define ITIMER_VIRTUAL 1    // CPU time in ring 3      -> SIGVTALRM
#define ITIMER_PROF    2    // CPU time, ring 3 and 0  -> SIGPROF

// Every timer tick: count each process's interval timers down, send the
// signal when one runs out, and load its interval again.
void signal_tick(void);

/* ---------- Delivery (deliver.c) ---------- */

// Called on the way back to ring 3: run a handler or a default action
// for the first deliverable signal. May end or stop the process.
void signal_deliver(registers_t *regs);

/* ---------- Queued signals (sigqueue.c) ---------- */

#define SIGQUEUE_SIZE 64    // queued siginfos, all processes together

// Queue 'info' for 'target' (rt_sigqueueinfo). The sender's user may
// hold only so many at once (sigqueue_limit). 1 = queued, 0 = refused.
int sigqueue_add(process_t *target, const k_siginfo_t *info);

// Take the oldest queued siginfo of 'sig' for 'p'. 1 = found.
int sigqueue_take(process_t *p, int sig, k_siginfo_t *out);

// Is one more siginfo of 'sig' queued for 'p'?
int sigqueue_waiting(const process_t *p, int sig);

// How many queued siginfos user 'uid' has sent and not yet delivered.
int sigqueue_count_user(uint32_t uid);

// Forget everything queued for 'p' (the reaper, before freeing it).
void sigqueue_release(process_t *p);

#endif
