# Defence: user_ptr_bounds

| | |
|---|---|
| **Stops** | [`user_ptr_wrap`](../../attack/user_ptr_wrap/) |
| **Protects** | trust: addresses from outside are checked before use |
| **Called from** | the `int 0x80` handler, for every address a ring 3 program passes in (buffers and path strings) |

## The rules

| Check | For | Rule |
|---|---|---|
| `def_user_range_ok` | buffers from ring 3 | inside user space (`size <= USER_SPACE_END - start`), and every page mapped with the USER bit |
| `def_user_string_ok` | strings (paths) from ring 3 | every byte up to the NUL is in mapped user memory, within a length limit |

## Why

A 32-bit `start + size` wraps around:

```
0xFFFFF000 + 0x2000 = 0x00001000     (not 0x100001000)
```

So the obvious check `start + size <= END` lets a range through that
really runs over the top of memory and back into the kernel. Comparing
`size` with `END - start` has no addition, so it cannot wrap.

Being inside user space is not enough either. If a page is not mapped,
the **kernel** page-faults while it copies, and a kernel page fault halts
the whole OS. So every page is checked with `vmm_walk()` first. A bad
pointer must only ever make the program's own call fail.

The old version also used `0xC0000000` as the start of "kernel space",
copied from Linux. But the UzOS kernel lives at 1 MiB, so kernel
addresses passed as "user". User space is now `0x08000000`–`0xC0000000`
(see `core/layout/memory_layout.h`).

## Uses

- `USER_SPACE_START` / `USER_SPACE_END` from core/layout, `vmm_walk()` from core/vmm
- `sec_log()` from the security lab

## Try it

```
seclab verify user_ptr_bounds
seclab attack user_ptr_wrap live
```
