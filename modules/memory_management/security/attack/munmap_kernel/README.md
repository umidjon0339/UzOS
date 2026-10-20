# Attack: munmap_kernel

| | |
|---|---|
| **Stopped by** | [`mmap_ownership`](../../defence/mmap_ownership/) |
| **Breaks** | isolation: you can only give back what you were given |
| **Severity** | critical |

## What it does

`munmap(addr, length)` unmaps pages **and frees their frames**. The old
version checked only that `addr` was below `0xC0000000`, which in UzOS
includes the kernel. So

```
munmap(0x00100000, 4096)
```

freed the frame holding the kernel's code. The next allocation got that
frame and overwrote the kernel.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the targets |
| `live` | calls the real `sys_munmap()` on the kernel code, the kernel heap, and half of a fresh mmap region (all must be refused, with no frame freed), then unmaps the whole region (allowed) |

## Uses

`sys_mmap()` / `sys_munmap()` from syscalls/, `pmm_get_used_blocks()` from core/pmm.

## Try it

```
seclab attack munmap_kernel live
seclab log
```
