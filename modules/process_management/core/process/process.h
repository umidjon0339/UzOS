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
 *                      ^                      |                     |
 *                      +--- SLEEPING <--sleep-+          reaper frees it
 */

#include <stdint.h>

#define MAX_PROCESSES      64
#define PROCESS_STACK_SIZE 4096     // each process gets one 4 KiB stack
#define PROCESS_NAME_LEN   32
#define MAX_FDS            16       // open files per process
#define PROCESS_RLIMITS    16       // resource limits, numbered as in Linux (cpu_management/core/rlimit)

struct vfs_node;

typedef enum {
    PROCESS_STATE_FREE,         // empty slot (0, so a zeroed slot is free)
    PROCESS_STATE_READY,        // waiting for its turn on the CPU
    PROCESS_STATE_RUNNING,      // on the CPU right now
    PROCESS_STATE_SLEEPING,     // waiting for sleep_ticks to reach 0
    PROCESS_STATE_ZOMBIE        // finished; the reaper will free the slot
} process_state_t;

typedef struct process {
    uint32_t pid;                   // 1, 2, 3, ... never reused
    char     name[PROCESS_NAME_LEN];
    process_state_t state;
    uint32_t uid;                   // owner: 0 = root
    int      is_system;             // 1 = part of the kernel: cannot be killed

    void   (*entry)(void);          // the function this process runs
    uint32_t stack_base;            // lowest address of its stack (0 = boot stack)
    uint32_t esp;                   // saved stack pointer while not running
    uint32_t sleep_ticks;           // timer ticks left while SLEEPING
    int      sleep_interruptible;   // 1 = a signal may end the sleep early

    uint32_t signals;               // pending signals: bit n = signal n (core/signal)

    // Files, used by filesystem_management and io_management.
    struct vfs_node *cwd;               // current directory
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

// The uid of whoever is running right now: the user a new process
// should belong to. 0 (root) before the scheduler exists.
uint32_t process_caller_uid(void);

process_t *process_get_by_pid(uint32_t pid);    // NULL if no such process
process_t *process_get_table(void);             // all MAX_PROCESSES slots
int process_count_free_slots(void);

#endif
