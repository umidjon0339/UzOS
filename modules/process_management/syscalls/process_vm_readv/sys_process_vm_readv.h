#ifndef SYS_PROCESS_VM_READV_H
#define SYS_PROCESS_VM_READV_H
/* sys_process_vm_readv.h - process_vm_readv(pid, local, n, remote, n, flags): copy memory from 'pid' into my buffers. */
#include <stdint.h>
#define SYS_PROCESS_VM_READV 94   // EAX=94, EBX=pid, ECX=local iov, EDX=count, ESI=remote iov, EDI=count, EBP=flags (0)
#define VM_IOV_MAX 8                // pieces per side

// struct iovec: one piece of memory.
typedef struct {
    uint32_t base;
    uint32_t len;
} vm_readv_iovec_t;

int sys_process_vm_readv(uint32_t pid, const vm_readv_iovec_t *local, uint32_t local_count,
                         const vm_readv_iovec_t *remote, uint32_t remote_count, uint32_t flags);
#endif
