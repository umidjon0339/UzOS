# Attack: tsc_timing

| | |
|---|---|
| **Stopped by** | [`tsc_restrict`](../../defence/tsc_restrict/) |
| **Breaks** | disclosure: timing must not reveal secrets |
| **Severity** | medium |

## What it does

The kernel does more work in some cases than in others. Looking up a
file that exists walks further through the directory than looking up
one that does not:

```
vfs_lookup("/hello")          2100 cycles     exists     (one run in QEMU;
vfs_lookup("/no_such_file")   1990 cycles     missing     your numbers will differ)
```

A program that can measure this gap learns whether the file exists
**without being allowed to look**. That is a **timing side channel**,
the same family as Spectre and cache attacks.

The measuring tool is `rdtsc`, the CPU's cycle counter. The ring 3
program [tsc_timing.s](tsc_timing.s) does:

```
rdtsc            ; start
int 0x80         ; getpid(): the kernel work to time
rdtsc            ; end
                 ; mailbox[1] = end - start
```

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | the kernel (ring 0, where `rdtsc` is allowed) times both lookups and shows the gap |
| `live` | runs the program as user 1; with the defence it is killed at its first `rdtsc` (`[CPU FAULT] #13 ... rdtsc`) |

## Uses

- `tsc_now()` / `tsc_cycles_to_ns()` from [core/tsc](../../../core/tsc/)
- `vfs_lookup()` from filesystem_management
- `usertask_start()` / `usertask_mailbox()` from process_management/core/usertask

## Try it

```
seclab attack tsc_timing
seclab attack tsc_timing live
faults
```
