# Defence: null_page

| | |
|---|---|
| **Stops** | [`null_deref`](../../attack/null_deref/) |
| **Protects** | consistency: a NULL pointer bug is caught, not hidden |
| **Called from** | `vmm_init()` in [core/vmm](../../../core/vmm/vmm.c) |

## The rule

Virtual page 0 (`0x0`–`0xFFF`) is never mapped.

## Why

The VMM identity-maps the first 8 MiB, so the kernel can use VGA memory
and its own image by their physical addresses. That used to include
page 0. A NULL pointer access then silently read or wrote **physical
address 0**, the BIOS interrupt table. Nothing complained; the damage
showed up much later, somewhere else.

With page 0 unmapped, the same bug causes an immediate page fault, and
the handler prints `[NULL pointer]` with the faulting address and `EIP`.
A crash at the bug is much easier to fix than corruption found later.

This is not a "rule on numbers" like the other defences. The selftest
checks the live page tables: is page 0 really unmapped?

## Uses

- `vmm_unmap_page()` / `vmm_walk()` from core/vmm
- `NULL_PAGE_END` from core/layout

## Try it

```
pagewalk 0                       # not mapped
seclab verify null_page
seclab attack null_deref live
```
