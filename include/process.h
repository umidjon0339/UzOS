#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 64
#define PROCESS_STACK_SIZE 4096 // 4KB
#define MAX_FDS 16
#define MAX_CHILDREN_PER_PROCESS 10 // Security limit to prevent fork bombs!

struct vfs_node;

typedef enum {
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_ZOMBIE,
    PROCESS_STATE_FREE // Slot is empty
} process_state_t;

typedef struct process {
    uint32_t pid;
    char name[32];
    process_state_t state;
    
    uint32_t esp; // Current stack pointer
    uint32_t stack_base; // Base of the dynamically allocated stack
    
    // Security and Hierarchy
    uint32_t uid; // User ID (0 = root)
    uint32_t parent_pid;
    uint32_t child_count;
    
    uint32_t sleep_ticks; // Ticks left if sleeping
    
    // File Descriptors
    struct vfs_node *cwd;
    struct vfs_node *fd_table[MAX_FDS];
    uint32_t fd_offset[MAX_FDS];
} process_t;

void process_init(void);

/* uid is set by the kernel at spawn time, never by the caller later.
 * 0 = root, anything else = unprivileged. */
int process_create(const char *name, void (*entry_point)(void), uint32_t uid);
int process_fork(void); // Implements fork with security limits
/* Returns 1 if the kill was accepted, 0 if a security guard refused it. */
int process_kill(uint32_t pid);
process_t *process_get_by_pid(uint32_t pid);
process_t *process_get_table(void);
void process_exit(void);

#endif
