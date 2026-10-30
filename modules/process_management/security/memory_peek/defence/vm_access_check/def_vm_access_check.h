#ifndef DEF_VM_ACCESS_CHECK_H
#define DEF_VM_ACCESS_CHECK_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// May 'caller' read or write [addr, addr + len) in 'target'? Only with
// ptrace rights over it (itself; or its own user's dumpable process; or
// CAP_PTRACE), never a system process, and only the target's USER
// memory - never the kernel's. process_vm_readv / writev and ptrace
// PEEK / POKE ask this. 1 = allowed, 0 = refused (logged).
int def_vm_access_ok(const process_t *caller, const process_t *target, uint32_t addr, uint32_t len);

#endif
