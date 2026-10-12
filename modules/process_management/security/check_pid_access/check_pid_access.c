#include "proc_security.h"

int proc_security_check_pid_access(uint32_t caller_pid, uint32_t target_pid) {
    // Basic isolation: currently everything is ring 0, so access is allowed.
    // In Phase 8, we will restrict tasks from killing other user tasks.
    (void)caller_pid;
    (void)target_pid;
    return 1; 
}
