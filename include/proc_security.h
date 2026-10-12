#ifndef PROC_SECURITY_H
#define PROC_SECURITY_H

#include <stdint.h>
#include "process.h"

int proc_security_check_pid_access(uint32_t caller_pid, uint32_t target_pid);
void proc_security_check_stack_overflow(process_t *p);

#endif
