# Defence: mmap_ownership

| | |
|---|---|
| **Stops** | [`munmap_kernel`](../../attack/munmap_kernel/) |
| **Protects** | isolation: you can only give back memory you were given |
| **Called from** | `sys_munmap()` in [syscalls/munmap](../../../syscalls/munmap/sys_munmap.c) |

## The rule

`munmap(start, length)` is allowed only if `start` is the start of a
region `mmap()` handed out, and `length` covers exactly that region.

## Why

`munmap()` unmaps the pages **and frees the frames behind them**. The
old version accepted any address below `0xC0000000`. So

```
munmap(0x00100000, 4096)
```

would free the frame holding the kernel's own code. The next
`pmm_alloc_block()` hands it out, and the new owner overwrites the
kernel.

`mmap()` now records every region it creates (`core/umem`), and
`munmap()` asks that list. Unmapping half a region is not supported, so
a partial unmap is refused too.

## Uses

- `umem_region_pages()` from core/umem
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab verify mmap_ownership
seclab attack munmap_kernel live
memtest                          # a real mmap/munmap pair still works
```
