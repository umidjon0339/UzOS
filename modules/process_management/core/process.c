#include "process.h"
#include "scheduler.h"
#include "kheap.h"
#include "string.h"
#include "vga.h"

static process_t process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_STATE_FREE;
        process_table[i].pid = 0;
    }
}

static int find_free_process_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_FREE) {
            return i;
        }
    }
    return -1;
}

// When a thread function returns, it comes here to die.
void process_exit(void) {
    // Ideally, we'd trigger a syscall. For now, since we are in ring 0,
    // we can just spin. The scheduler handles killing it properly if we implement an exit syscall.
    // To make it simple without an exit syscall just yet, we'll just infinite halt.
    // Or we could have `sys_exit` directly called.
    // Let's just halt for now.
    while (1) {
        asm volatile("hlt");
    }
}

int process_create(const char *name, void (*entry_point)(void)) {
    int slot = find_free_process_slot();
    if (slot == -1) {
        return -1; // No free slots
    }
    
    process_t *p = &process_table[slot];
    p->pid = next_pid++;
    
    strncpy(p->name, name, 31);
    p->name[31] = '\0';
    
    p->state = PROCESS_STATE_READY;
    p->sleep_ticks = 0;
    
    // Security Init
    p->uid = 0; // Everything runs as root for now
    p->parent_pid = 0;
    p->child_count = 0;
    
    p->stack_base = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
    if (!p->stack_base) {
        p->state = PROCESS_STATE_FREE;
        return -1;
    }
    
    // Set up initial stack for context switch.
    uint32_t *stack = (uint32_t *)(p->stack_base + PROCESS_STACK_SIZE);
    
    // Push the return address (so when the thread returns, it goes to process_exit)
    *(--stack) = (uint32_t)process_exit;
    
    // Push the actual entry point (popped by task_start_wrapper's ret)
    *(--stack) = (uint32_t)entry_point;
    
    extern void task_start_wrapper(void);
    // Push the wrapper (popped by switch_task's ret)
    *(--stack) = (uint32_t)task_start_wrapper;

    
    // Push initial registers (ebp, ebx, esi, edi)
    *(--stack) = 0; // ebp
    *(--stack) = 0; // ebx
    *(--stack) = 0; // esi
    *(--stack) = 0; // edi
    
    p->esp = (uint32_t)stack;
    
    return p->pid;
}

int process_fork(void) {
    process_t *current_process = scheduler_get_current_process();
    if (!current_process) return -1;
    
    // ==========================================
    // FORK BOMB SECURITY LIMIT
    // ==========================================
    if (current_process->child_count >= MAX_CHILDREN_PER_PROCESS && current_process->uid != 0) {
        vga_print_str("\n[SECURITY] Fork Bomb prevented! Process reached child limit.\n");
        return -1; // Deny fork
    }
    
    int slot = find_free_process_slot();
    // Reserve the last 5 slots for ROOT (UID 0) to prevent total system lockout
    if (slot >= MAX_PROCESSES - 5 && current_process->uid != 0) {
        vga_print_str("\n[SECURITY] Fork prevented! Reserved for Root.\n");
        return -1;
    }
    
    if (slot == -1) return -1;
    
    // We increment the parent's child counter!
    current_process->child_count++;
    
    process_t *child = &process_table[slot];
    child->pid = next_pid++;
    child->parent_pid = current_process->pid;
    child->uid = current_process->uid;
    child->child_count = 0;
    child->state = PROCESS_STATE_READY;
    child->sleep_ticks = 0;
    
    strncpy(child->name, current_process->name, 31);
    
    // Allocate child stack
    child->stack_base = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
    
    // TODO: True fork requires copying the Page Directory and the hardware context (Registers).
    // This will be finalized in the next step when we map Ring 3 (User Space).
    // For now, return error so we don't crash.
    vga_print_str("Fork security limit checked! Ready for Ring 3 clone.\n");
    
    return -1; // Not fully implemented yet until Syscalls are up!
}

void process_kill(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_STATE_FREE) {
            process_table[i].state = PROCESS_STATE_ZOMBIE;
            kfree((void *)process_table[i].stack_base);
            process_table[i].state = PROCESS_STATE_FREE;
            return;
        }
    }
}

process_t *process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_STATE_FREE) {
            return &process_table[i];
        }
    }
    return NULL;
}

process_t *process_get_table(void) {
    return process_table;
}
