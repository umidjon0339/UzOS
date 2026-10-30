#ifndef SYS_PROCESS_VM_WRITEV_H
#define SYS_PROCESS_VM_WRITEV_H
/* sys_process_vm_writev.h - process_vm_writev(pid, local, n, remote, n, flags): copy memory from my buffers into 'pid'. */
#include <stdint.h>
#define SYS_PROCESS_VM_WRITEV 95   // EAX=95, EBX=pid, ECX=local iov, EDX=count, ESI=remote iov, EDI=count, EBP=flags (0)
#define VM_IOV_MAX 8                // pieces per side

// struct iovec: one piece of memory.
typedef struct {
    uint32_t base;
    uint32_t len;
} vm_writev_iovec_t;

int sys_process_vm_writev(uint32_t pid, const vm_writev_iovec_t *local, uint32_t local_count,
                         const vm_writev_iovec_t *remote, uint32_t remote_count, uint32_t flags);
#endif
