# Attack: null_deref

| | |
|---|---|
| **Stopped by** | [`null_page`](../../defence/null_page/) |
| **Breaks** | consistency: bugs show up instead of corrupting memory |
| **Severity** | medium |

## What it does

```c
struct thing *p = NULL;     // forgot to set it
p->count = 5;               // writes to address 0 + offset
```

When page 0 is mapped (it used to be, as part of the identity-mapped
first 8 MiB), this write succeeds. It overwrites the BIOS interrupt
table at physical address 0, and nothing reports it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | walks the page tables for `NULL + 0x0`, `0x4`, `0x400`, `0xFFC` |
| `live` | the same check, with a verdict |

`live` does not really dereference NULL. In the kernel that page fault
would, correctly, halt the whole OS. Inside a user program it only kills
that program; see the page fault handler in `core/vmm`.

## Uses

`vmm_walk()` from core/vmm.

## Try it

```
pagewalk 0
seclab attack null_deref live
seclab verify null_page
```
