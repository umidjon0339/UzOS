# Memory Management Module

## Subdirectory Architecture

```
modules/memory_management/
├── core/                  # Core PMM, VMM Paging & Kernel Heap Engines
│   ├── pmm.c              # Physical Memory Manager (Bitmap Page Frame Allocator)
│   ├── vmm.c              # Virtual Memory Manager (2-Level Paging, CR3/CR0)
│   └── kheap.c            # Kernel Heap Allocator (kmalloc, kfree)
├── system_calls/          # Memory System Calls for User Space
│   └── mem_syscalls.c     # sys_sbrk(), sys_mmap(), sys_munmap()
├── commands/              # Inspection & Debug Commands
│   └── mem_commands.c     # cmd_meminfo(), cmd_heapinfo()
├── security/              # Pointer Validation & Privilege Boundary Checks
│   └── mem_security.c     # mem_validate_user_ptr(), mem_is_kernel_address()
└── README.md
```

## Subsystem Highlights
- **core/**: Implements physical page frame tracking (`pmm.c`), virtual page table mappings (`vmm.c`), and dynamic kernel heap allocation (`kheap.c`).
- **system_calls/**: Provides system call handlers (`sys_mmap`, `sys_sbrk`) to allow user programs to request dynamic memory from the kernel.
- **commands/**: Shell commands to monitor RAM status (`cmd_meminfo`) and heap status (`cmd_heapinfo`).
- **security/**: Memory validation routines to prevent user space programs from passing invalid or malicious kernel addresses into system calls.
