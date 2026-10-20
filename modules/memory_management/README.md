# memory_management — physical frames, paging, heaps, mmap

Memory in UzOS has three layers, each built on the one below:

```
kmalloc / kfree              kernel heap: any size        core/kheap
sbrk / mmap / munmap         user pages for programs      core/umem + syscalls/
        |
vmm_map_page                 virtual page -> frame        core/vmm
        |
pmm_alloc_block              4 KiB frames of real RAM     core/pmm
```

```
memory_management/
├── core/
│   ├── layout/  memory_layout.h   the address-space map (read this first)
│   ├── pmm/     pmm.h/.c          physical frames: a bitmap, 1 bit per 4 KiB
│   ├── vmm/     vmm.h/.c          page tables, map/unmap/walk, page faults
│   ├── kheap/   kheap.h/.c        kmalloc/kfree: first-fit block list
│   └── umem/    umem.h/.c         map/unmap user pages, list of mmap regions
├── commands/    one folder each
│   ├── mem_commands.h/.c          the command table
│   └── meminfo/ heapinfo/ pagewalk/ memtest/
├── syscalls/    one folder each: number + .h + .c
│   └── sbrk/ mmap/ munmap/
└── security/
    ├── attack/<id>/   atk_<id>.c + README.md
    └── defence/<id>/  def_<id>.h/.c + README.md
```

## The address space

All processes share one address space for now
([memory_layout.h](core/layout/memory_layout.h)):

| Virtual address | What | Mapped to |
|---|---|---|
| `0x00000000` | NULL page | **nothing**: NULL accesses fault |
| `0x00001000`–`0x007FFFFF` | BIOS, VGA, kernel at 1 MiB | the same physical address |
| `0x01000000`–`0x017FFFFF` | kernel heap | any free frames |
| `0x08000000`–`0xBFFFFFFF` | **user space** | frames mapped on demand |
| ` ` `0x08048000` | ELF programs (`exec`) | |
| ` ` `0x40000000` | `testuser` code + stack | |
| ` ` `0x50000000` | `sbrk` heap | |
| ` ` `0x60000000` | `mmap` regions | |
| `0xFD000000` | GUI framebuffer | the same physical address |
| `0xFFC00000` | the page tables themselves | recursive mapping |

## How an address is translated

```
virtual 0x01000000 = 0000000100 | 0000000000 | 000000000000
                     dir #4       table #0     offset 0
CR3 -> directory[4] -> page table -> entry[0] -> frame 0x00152000 -> + offset
```

Try it: `pagewalk 0x1000000`, `pagewalk 0xb8000`, `pagewalk 0`.

**The recursive trick.** A new page table is a frame from anywhere in
RAM, but the kernel can only reach the first 8 MiB by physical address.
So the last directory entry points at the directory itself. Then table
`i` always appears at `0xFFC00000 + i * 4096`.

## The kernel heap

```
[hdr|  used  ][hdr| free ][hdr|   used   ][hdr|        free        ]
 magic size is_free next
```

`kmalloc` walks the list and takes the first free block that fits,
splitting off the rest. `kfree` marks it free and merges free
neighbours. Both turn interrupts off, because the reaper frees process
stacks from the timer interrupt.

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 5 | `sbrk`   | EBX = bytes (may be negative) | old break, or `-1` |
| 6 | `mmap`   | EBX = length | address of zeroed pages, or `0` |
| 7 | `munmap` | EBX = address, ECX = length | `0`, or `-1` |

They are routed by `process_management/syscalls/handler`. `testuser`
calls `mmap` and `munmap` from ring 3; `memtest` calls all three from
the shell.

## Commands

| Command | What it does | Uses |
|---|---|---|
| `meminfo` | frames used/free + the bootloader's memory map | `pmm_*` |
| `heapinfo` | heap blocks, bytes, largest free block, health | `kheap_get_stats` |
| `pagewalk <addr>` | follow an address through the page tables | `vmm_walk` |
| `memtest` | sbrk, mmap, munmap; checks every frame comes back | `sys_*`, `pmm_*` |

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `user_ptr_wrap` | trust | `user_ptr_bounds` | `int 0x80` (munmap), io read/write |
| `mmap_length_wrap` | resource | `mmap_page_count` | `sys_mmap`, `sys_munmap` |
| `pmm_bad_free` | consistency | `pmm_free_guard` | `pmm_free_block` |
| `heap_double_free` | consistency | `heap_guard` | `kfree` |
| `heap_overflow` | consistency | `heap_canary` | `kmalloc`, `kfree` |
| `munmap_kernel` | isolation | `mmap_ownership` | `sys_munmap` |
| `null_deref` | consistency | `null_page` | `vmm_init` |

Each folder has a README. Try `seclab attack <id> live` and `seclab verify all`.

## Page faults

- **In a user program** (ring 3): print the address and reason, and end
  only that process (`testuser null` shows it).
- **In the kernel:** a kernel bug. Print everything, including
  `[NULL pointer]` for addresses in page 0, and halt.

## Known limits

- **One address space** for all processes: one page directory, one
  `sbrk` break, one list of mmap regions.
- **No partial `munmap`**: only whole regions mmap handed out.
- **The kernel heap has a fixed size** (8 MiB) and does not grow.
