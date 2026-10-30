/*
 * signal.c - Send signals, name them, count alarms down.
 *
 * signal_send() is called from the keyboard interrupt while the target
 * may be halfway into going to sleep. Interrupts stay off while the bit
 * and the state change together, so the wake-up can never be missed
 * (sys_sleep_interruptible does the same on its side).
 *
 * Three signals act at once instead of waiting to be delivered:
 *   SIGSTOP   the process becomes STOPPED: the scheduler skips it
 *   SIGCONT   a STOPPED process carries on (sleeping or ready)
 *   ignored   SIG_IGN, or a default action of "ignore": thrown away now,
 *             as Linux does, so it can never wake a sleep
 *
 * Every timer tick, signal_tick() runs the interval timers (setitimer,
 * alarm): REAL counts wall-clock time, VIRTUAL and PROF only the CPU the
 * process used (as charged by cpu_management's accounting).
 *
 * Uses: the current process (core/scheduler), cpu_irq_save (arch/x86/cpu),
 *       isr_on_return_to_user (arch/x86/isr) for delivery (deliver.c),
 *       strcmp (lib/string), atoi (lib/stdlib).
 */

#include "modules/process_management/core/signal/signal.h"        // this file
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process, scheduler_yield
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                    // strcmp, strncmp
#include "lib/stdlib/stdlib.h"                                    // atoi

static const struct {
    const char  *name;
    sig_action_t action;
} signals[SIGNAL_COUNT] = {
    [SIGHUP]  = { "HUP",  ACT_TERM },   [SIGINT]    = { "INT",    ACT_TERM },
    [SIGQUIT] = { "QUIT", ACT_CORE },   [SIGILL]    = { "ILL",    ACT_CORE },
    [SIGTRAP] = { "TRAP", ACT_CORE },   [SIGABRT]   = { "ABRT",   ACT_CORE },
    [SIGBUS]  = { "BUS",  ACT_CORE },   [SIGFPE]    = { "FPE",    ACT_CORE },
    [SIGKILL] = { "KILL", ACT_TERM },   [SIGUSR1]   = { "USR1",   ACT_TERM },
    [SIGSEGV] = { "SEGV", ACT_CORE },   [SIGUSR2]   = { "USR2",   ACT_TERM },
    [SIGPIPE] = { "PIPE", ACT_TERM },   [SIGALRM]   = { "ALRM",   ACT_TERM },
    [SIGTERM] = { "TERM", ACT_TERM },   [SIGSTKFLT] = { "STKFLT", ACT_TERM },
    [SIGCHLD] = { "CHLD", ACT_IGNORE }, [SIGCONT]   = { "CONT",   ACT_CONT },
    [SIGSTOP] = { "STOP", ACT_STOP },   [SIGTSTP]   = { "TSTP",   ACT_STOP },
    [SIGTTIN] = { "TTIN", ACT_STOP },   [SIGTTOU]   = { "TTOU",   ACT_STOP },
    [SIGURG]  = { "URG",  ACT_IGNORE }, [SIGXCPU]   = { "XCPU",   ACT_CORE },
    [SIGXFSZ] = { "XFSZ", ACT_CORE },   [SIGVTALRM] = { "VTALRM", ACT_TERM },
    [SIGPROF] = { "PROF", ACT_TERM },   [SIGWINCH]  = { "WINCH",  ACT_IGNORE },
    [SIGIO]   = { "IO",   ACT_TERM },   [SIGPWR]    = { "PWR",    ACT_TERM },
    [SIGSYS]  = { "SYS",  ACT_CORE },
};

static int valid(int sig) {
    return sig >= 1 && sig < SIGNAL_COUNT;
}

static void deliver_hook(registers_t *regs) {
    signal_deliver(regs);
}

void signal_init(void) {
    isr_on_return_to_user(deliver_hook);
}

/* ---------- Stop and continue ---------- */

static void stop(process_t *p) {
    if (p->is_system || p->state == PROCESS_STATE_FREE ||
        p->state == PROCESS_STATE_ZOMBIE || p->state == PROCESS_STATE_STOPPED)
        return;                         // the kernel's own processes never freeze
    p->state = PROCESS_STATE_STOPPED;   // a sleep keeps its sleep_ticks for later
}

static void resume(process_t *p) {
    if (p->state == PROCESS_STATE_STOPPED)
        p->state = p->sleep_ticks ? PROCESS_STATE_SLEEPING : PROCESS_STATE_READY;
}

// Would 'p' throw 'sig' away right now?
static int ignored(const process_t *p, int sig) {
    uint32_t handler = p->sig_handler[sig];
    if (sig == SIGKILL || sig == SIGSTOP)
        return 0;
    return handler == SIG_IGN ||
           (handler == SIG_DFL && (signals[sig].action == ACT_IGNORE ||
                                   signals[sig].action == ACT_CONT));
}

/* ---------- Sending ---------- */

void signal_send_info(process_t *p, int sig, int code, uint32_t value) {
    (void)value;                        // queued values live in sigqueue.c
    if (!p || !valid(sig))
        return;

    process_t *sender = scheduler_get_current_process();
    uint32_t flags = cpu_irq_save();

    if (sig == SIGCONT || sig == SIGKILL) {
        p->signals &= ~(SIG_BIT(SIGSTOP) | SIG_BIT(SIGTSTP) | SIG_BIT(SIGTTIN) | SIG_BIT(SIGTTOU));
        resume(p);
    }
    if (signals[sig].action == ACT_STOP)
        p->signals &= ~SIG_BIT(SIGCONT);

    if (sig == SIGSTOP) {
        stop(p);                        // never pending: it acts now
    } else if (!ignored(p, sig)) {
        p->signals |= SIG_BIT(sig);
        p->sig_from[sig] = code == SI_KERNEL || !sender ? 0 : sender->pid;
        p->sig_code[sig] = code;
        if (p->state == PROCESS_STATE_SLEEPING && p->sleep_interruptible &&
            (SIG_BIT(sig) & ~p->sig_blocked)) {
            p->sleep_ticks = 0;
            p->state = PROCESS_STATE_READY;     // wake it: it may run next tick
        }
    }
    cpu_irq_restore(flags);

    // Stopped ourselves (kill(getpid(), SIGSTOP)): give the CPU up now.
    if (p == sender && p->state == PROCESS_STATE_STOPPED)
        scheduler_yield();
}

void signal_send(process_t *p, int sig) {
    signal_send_info(p, sig, SI_USER, 0);
}

int signal_pending(const process_t *p, int sig) {
    if (!p || !valid(sig))
        return 0;
    return (p->signals >> sig) & 1;
}

void signal_clear(process_t *p, int sig) {
    if (!p || !valid(sig))
        return;
    uint32_t flags = cpu_irq_save();
    p->signals &= ~SIG_BIT(sig);
    cpu_irq_restore(flags);
}

uint32_t signal_deliverable(const process_t *p) {
    return p ? p->signals & ~(p->sig_blocked & ~SIG_UNBLOCKABLE) : 0;
}

int signal_interrupted(void) {
    return signal_pending(scheduler_get_current_process(), SIGINT);
}

/* ---------- Names ---------- */

int signal_number(const char *name) {
    if (name[0] >= '0' && name[0] <= '9') {
        int n = atoi(name);
        return valid(n) ? n : 0;
    }
    if (strncmp(name, "SIG", 3) == 0)
        name += 3;
    for (int sig = 1; sig < SIGNAL_COUNT; sig++)
        if (strcmp(name, signals[sig].name) == 0)
            return sig;
    return 0;
}

const char *signal_name(int sig) {
    return valid(sig) ? signals[sig].name : "?";
}

sig_action_t signal_default_action(int sig) {
    return valid(sig) ? signals[sig].action : ACT_IGNORE;
}

/* ---------- Interval timers: setitimer(), alarm() ---------- */

static const int timer_signal[ITIMERS] = { SIGALRM, SIGVTALRM, SIGPROF };

// Count timer 'which' of 'p' down by 'ticks'; fire it at 0.
static void count_down(process_t *p, int which, uint32_t ticks) {
    uint32_t *left = &p->itimer_value[which];
    if (!*left || !ticks)
        return;
    if (*left > ticks) {
        *left -= ticks;
        return;
    }
    *left = p->itimer_interval[which];      // periodic: again; one-shot: off
    signal_send_info(p, timer_signal[which], SI_TIMER, 0);
}

void signal_tick(void) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        count_down(p, ITIMER_REAL, 1);      // wall clock: everyone, every tick
    }

    // CPU clocks: only the running process used CPU on this tick, and
    // accounting (cpu_management) has just charged it to ring 3 or ring 0.
    process_t *p = scheduler_get_current_process();
    if (!p)
        return;
    uint32_t user   = p->user_ticks - p->itimer_seen_user;
    uint32_t kernel = p->kernel_ticks - p->itimer_seen_kernel;
    p->itimer_seen_user   = p->user_ticks;
    p->itimer_seen_kernel = p->kernel_ticks;
    count_down(p, ITIMER_VIRTUAL, user);
    count_down(p, ITIMER_PROF, user + kernel);
}
