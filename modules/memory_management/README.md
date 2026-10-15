# Memory Management

## Layout

```text
memory_management/
├── core/                 # PMM, VMM, and kernel heap
├── commands/             # Memory inspection commands
├── system_calls/         # mmap, munmap, and sbrk
└── security/
    ├── attack/           # Safe security demonstrations
    └── defence/          # Policies used by the kernel and their selftests
```

## Security pairs

| Attack | Defence | Purpose |
|---|---|---|
| `user_ptr_wrap` | `user_ptr_bounds` | Keep user buffers below the kernel boundary, including at integer limits. |
| `mmap_length_wrap` | `mmap_page_count` | Round mapping lengths to pages without overflowing. |
| `pmm_free_oob` | `pmm_free_bounds` | Reject unaligned or out-of-range frame releases before touching the bitmap. |

The demonstrations are registered in the `seclab` command alongside
the process-management pairs. `simulate` only probes policies; `live`
uses the same checks that protect real memory operations.

```text
seclab list
seclab attack user_ptr_wrap
seclab attack mmap_length_wrap live
seclab attack pmm_free_oob live
seclab verify-all
```

## Core components

- `core/pmm.c` tracks physical page frames with a bitmap.
- `core/vmm.c` manages virtual-memory page tables.
- `core/kheap.c` provides kernel heap allocation.
- `system_calls/` exposes memory allocation operations to user space.
