/*
 * sys_process_vm_writev.c - process_vm_writev(pid, local, n, remote, n, flags).
 *
 * Copy memory INTO process 'pid': my local pieces, in order, fill
 * its remote pieces, until either side ends.
 * Returns the bytes copied, or -1. Every remote piece is checked first
 * (vm_access_check: ptrace rights, its user memory only); local pieces
 * from ring 3 are checked by the syscall handler (user_ptr_bounds).
 * UzOS has one address space, so a copy is a memcpy.
 *
 * Uses: scheduler_get_current_process / process_get_by_pid (core/process),
 *       memcpy (lib/string), def_vm_access_ok (security/memory_peek/
 *       defence/vm_access_check).
 */
#include "modules/process_management/syscalls/process_vm_writev/sys_process_vm_writev.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                         // scheduler_get_current_process
#include "modules/process_management/security/memory_peek/defence/vm_access_check/def_vm_access_check.h"  // def_vm_access_ok
#include "lib/string/string.h"                                                           // memcpy

int sys_process_vm_writev(uint32_t pid, const vm_writev_iovec_t *local, uint32_t local_count,
                         const vm_writev_iovec_t *remote, uint32_t remote_count, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(pid);
    if (!self || !t || flags || local_count > VM_IOV_MAX || remote_count > VM_IOV_MAX)
        return -1;
    // DEFENCE vm_access_check: every remote piece, before any byte moves.
    for (uint32_t i = 0; i < remote_count; i++)
        if (remote[i].len && !def_vm_access_ok(self, t, remote[i].base, remote[i].len))
            return -1;

    uint32_t copied = 0, li = 0, lo = 0, ri = 0, ro = 0;
    while (li < local_count && ri < remote_count) {
        uint32_t n = local[li].len - lo;
        if (remote[ri].len - ro < n)
            n = remote[ri].len - ro;
        memcpy((void *)(remote[ri].base + ro), (const void *)(local[li].base + lo), n);
        copied += n;
        if ((lo += n) == local[li].len)  { li++; lo = 0; }
        if ((ro += n) == remote[ri].len) { ri++; ro = 0; }
    }
    return (int)copied;
}
