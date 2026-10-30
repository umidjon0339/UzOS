#ifndef PROCESS_H
#define PROCESS_H

/*
 * process.h - The process table.
 *
 * A process is a function with its own stack that the scheduler runs in
 * turns with the others. For each one the kernel keeps a Process Control
 * Block (PCB), process_t, in a fixed table of MAX_PROCESSES slots.
 *
 * Life of a process:
 *
 *   FREE --create--> READY <--scheduler--> RUNNING --exit/kill--> ZOMBIE
 *                    ^  ^                     |                     |
 *                    |  +--- SLEEPING <--sleep-+          reaper frees it
 *                    +------ STOPPED  <--SIGSTOP (SIGCONT goes back)
 */

#include <stdint.h>

#define MAX_PROCESSES      64
#define PROCESS_STACK_SIZE 4096     // each process gets one 4 KiB stack
#define PROCESS_NAME_LEN   32
#define MAX_FDS            16       // open files per process
#define PROCESS_RLIMITS    16       // resource limits, numbered as in Linux (cpu_management/core/rlimit)
#define SIGNAL_COUNT       32       // signals 1..31, one bit each (core/signal)
#define SIGNAL_MAX_FRAMES  4        // handlers running inside each other, at most
#define ITIMERS            3        // ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF (core/signal)

struct vfs_node;

typedef enum {
    PROCESS_STATE_FREE,         // empty slot (0, so a zeroed slot is free)
    PROCESS_STATE_READY,        // waiting for its turn on the CPU
    PROCESS_STATE_RUNNING,      // on the CPU right now
    PROCESS_STATE_SLEEPING,     // waiting for sleep_ticks to reach 0
    PROCESS_STATE_ZOMBIE,       // finished; the reaper will free the slot
    PROCESS_STATE_STOPPED       // frozen by SIGSTOP / Ctrl+Z until SIGCONT
} process_state_t;

typedef struct process {
    uint32_t pid;                   // 1, 2, 3, ... never reused
    uint32_t generation;            // how often this SLOT has been used (pcb_generation)
    char     name[PROCESS_NAME_LEN];
    process_state_t state;
    uint32_t uid;                   // owner: 0 = root
    int      is_system;             // 1 = part of the kernel: cannot be killed

    void   (*entry)(void);          // the function this process runs
    uint32_t stack_base;            // lowest address of its stack (0 = boot stack)
    uint32_t esp;                   // saved stack pointer while not running
    uint32_t sleep_ticks;           // timer ticks left while SLEEPING
    int      sleep_interruptible;   // 1 = a signal may end the sleep early

    // Signals (core/signal). Bit n of a mask = signal n.
    uint32_t signals;               // pending: sent, not yet dealt with
    uint32_t sig_blocked;           // held back until unblocked (rt_sigprocmask)
    uint32_t sig_handler[SIGNAL_COUNT];  // SIG_DFL, SIG_IGN, or a ring 3 address (rt_sigaction)
    uint32_t sig_flags[SIGNAL_COUNT];    // SA_SIGINFO, SA_ONSTACK, ...
    uint32_t sig_restorer[SIGNAL_COUNT]; // where a handler returns to (SA_RESTORER)
    uint32_t sig_mask[SIGNAL_COUNT];     // also blocked while the handler runs
    uint32_t sig_from[SIGNAL_COUNT];     // who sent it (pid; 0 = the kernel)
    int32_t  sig_code[SIGNAL_COUNT];     // why: SI_USER, SI_KERNEL, ...
    uint32_t sig_frame[SIGNAL_MAX_FRAMES]; // where each running handler's frame is
    int      sig_frames;            // how many handlers are running right now
    uint32_t sig_saved_mask;        // rt_sigsuspend: the mask to put back
    int      sig_restore_mask;      // 1 = put sig_saved_mask back after delivery
    uint32_t altstack_sp, altstack_size, altstack_flags;  // sigaltstack()
    uint32_t itimer_value[ITIMERS];    // setitimer() / alarm(): ticks left, 0 = off
    uint32_t itimer_interval[ITIMERS]; // reloaded when it fires (0 = once)
    uint32_t itimer_seen_user;         // user/kernel ticks at the last timer tick:
    uint32_t itimer_seen_kernel;       // VIRTUAL and PROF count only the CPU it used
    int      exit_status;           // how it ended, as wait() reports it:
                                    // exit(n) = n << 8, killed by signal s = s

    // prctl() settings (syscalls/prctl).
    int      pdeathsig;             // signal it gets when its parent ends (0 = none)
    int      dumpable;              // 1 = may be inspected / dumped (set at create)
    int      no_new_privs;          // 1 = exec can never give it more rights (sticky)
    int      child_subreaper;       // 1 = orphaned descendants come to it, not to PID 1

    // vfork(): the parent waiting until this child execs or ends (0 = none).
    uint32_t vfork_parent;

    // Debugging (syscalls/ptrace, commands/strace).
    uint32_t tracer;                // pid of the process ptrace()ing it (0 = none)
    int      strace;                // 1 = print each of its syscalls (inherited)

    // seccomp() sandbox (syscalls/seccomp, defence seccomp_filter).
    int      seccomp_mode;          // 0 off, 1 strict, 2 filter
    uint32_t seccomp_allow[4];      // filter: bit n = syscall n allowed (0..127)
    uint32_t seccomp_action;        // filter: what a refused call gets

    // Files, used by filesystem_management and io_management.
    struct vfs_node *cwd;               // current directory
    uint32_t umask;                     // mode bits a new file does NOT get (umask)
    struct vfs_node *fd_table[MAX_FDS]; // open file behind each descriptor
    uint32_t fd_offset[MAX_FDS];        // where the next read/write happens
    uint32_t fd_flags[MAX_FDS];         // how it was opened (O_READ, O_WRITE, ...)

    // Family, set by cpu_management/core/task when the process is made.
    uint32_t ppid;                  // the process that created it (0 = none)
    uint32_t tgid;                  // thread group: its own pid, or the pid of
                                    // the thread that clone()d it (cpu_management syscalls/clone)
    uint32_t pgid;                  // process group id (setpgid/getpgid)
    uint32_t sid;                   // session id (setsid/getsid)
    uint32_t start_tick;            // timer tick it was created at

    // Scheduling, decided by cpu_management/core/priority and core/policy.
    int      priority;              // 0 = idle task only, 1 (low) .. 7 (high); follows nice
    int      nice;                  // -20 (most CPU) .. 19 (least CPU); 0 = normal
    int      policy;                // SCHED_OTHER, SCHED_FIFO, ... (core/policy)
    int      rt_priority;           // 1..99 for SCHED_FIFO / SCHED_RR, else 0
    int      reset_on_fork;         // 1 = its children start as SCHED_OTHER, nice 0
    uint32_t slice_left;            // ticks left of its current turn
    uint32_t waited_ticks;          // ticks READY without the CPU (aging)
    uint32_t cpu_mask;              // CPUs it may run on: bit n = CPU n (core/affinity)

    // CPU time, counted by cpu_management/core/accounting.
    uint32_t user_ticks;            // ticks it was running in ring 3
    uint32_t kernel_ticks;          // ticks it was running in ring 0
    uint32_t window_ticks;          // ticks so far in the current second
    uint32_t cpu_percent;           // its share of the CPU in the last second
    uint32_t ready_ticks;           // ticks it was READY but someone else ran
    uint32_t nvcsw;                 // context switches: it gave the CPU up (slept, ended)
    uint32_t nivcsw;                // context switches: it was preempted
    uint64_t cycles;                // TSC cycles it had the CPU (exact, not sampled)
    uint64_t cycles_since;          // TSC value when it last got the CPU

    // What its finished children used (added by core/task when they are freed).
    uint32_t child_user_ticks, child_kernel_ticks;
    uint32_t child_nvcsw, child_nivcsw;
    uint64_t child_cycles;

    // Resource limits (cpu_management/core/rlimit): soft (cur) and hard (max).
    uint32_t rlim_cur[PROCESS_RLIMITS];
    uint32_t rlim_max[PROCESS_RLIMITS];

    // Memory placement (cpu_management/core/numa).
    int      mempolicy;             // MPOL_DEFAULT, MPOL_BIND, ...
    uint32_t mempolicy_nodes;       // bit n = NUMA node n

    int      membarrier_registered; // membarrier(): private expedited allowed
} process_t;

// Mark every slot FREE. Called by scheduler_init().
void process_init(void);

// Start 'entry' as a new READY process owned by 'uid'.
// entry = NULL means "the code that is already running" (kernel_main):
// it gets a slot but no new stack. Returns the PID, or -1 if refused.
int process_create(const char *name, void (*entry)(void), uint32_t uid);

// Stop a process. Returns 1 if done, 0 if refused (see kill_guard).
int process_kill(uint32_t pid);

// End the calling process. Never returns.
void process_exit(void) __attribute__((noreturn));

// The same, recording how it ended (exit_status): exit(n) passes n << 8,
// a deadly signal s passes s (| 0x80 if Linux would write a core dump).
void process_exit_with(int status) __attribute__((noreturn));

// What happens around a process that ends (exit or kill), in one place:
// its children are re-parented (to the nearest child_subreaper above,
// else PID 1) and get their pdeathsig; processes it traced are let go;
// its parent gets SIGCHLD.
// Called by process_kill() and process_exit_with().
void process_ended(process_t *p);

// The session 'sid' hangs up (logout: its terminal is gone). Every
// process in it except 'keep' and system processes gets SIGHUP: one
// that ignores it (nohup) carries on, one with a handler runs it, any
// other ends (exit status SIGHUP). Returns how many ended.
int process_hangup(uint32_t sid, const process_t *keep);

// The uid of whoever is running right now: the user a new process
// should belong to. 0 (root) before the scheduler exists.
uint32_t process_caller_uid(void);

process_t *process_get_by_pid(uint32_t pid);    // NULL if no such process
process_t *process_get_table(void);             // all MAX_PROCESSES slots
int process_count_free_slots(void);

#endif
