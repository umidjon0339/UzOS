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

    // Files, used by filesystem_management and io_management.
    struct vfs_node *cwd;               // current directory
    struct vfs_node *fd_table[MAX_FDS]; // open file behind each descriptor
    uint32_t fd_offset[MAX_FDS];        // where the next read/write happens
    uint32_t fd_flags[MAX_FDS];         // how it was opened (O_READ, O_WRITE, ...)

    // Scheduling, decided by cpu_management/core/priority.
    int      priority;              // 0 = idle task only, 1 (low) .. 7 (high)
    uint32_t slice_left;            // ticks left of its current turn
    uint32_t waited_ticks;          // ticks READY without the CPU (aging)

    // CPU time, counted by cpu_management/core/accounting on every tick.
    uint32_t user_ticks;            // ticks it was running in ring 3
    uint32_t kernel_ticks;          // ticks it was running in ring 0
    uint32_t window_ticks;          // ticks so far in the current second
    uint32_t cpu_percent;           // its share of the CPU in the last second
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

process_t *process_get_by_pid(uint32_t pid);    // NULL if no such process
process_t *process_get_table(void);             // all MAX_PROCESSES slots
int process_count_free_slots(void);

#endif
