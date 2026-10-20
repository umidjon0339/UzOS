# Defence: pmm_free_guard

| | |
|---|---|
| **Stops** | [`pmm_bad_free`](../../attack/pmm_bad_free/) |
| **Protects** | consistency: each frame has exactly one owner |
| **Called from** | `pmm_free_block()` in [core/pmm](../../../core/pmm/pmm.c) |

## The rules

A frame may be freed only if it is:

1. **aligned**: the address is a multiple of 4096
2. **real RAM**: inside the range the bitmap covers
3. **not reserved**: not the kernel image, firmware, or boot data
4. **in use**: freeing a free frame is a double free

## Why

The next `pmm_alloc_block()` hands out whatever frame was freed. If the
kernel's own frame at `0x100000` could be freed, the next process stack
or page table would be written right over the kernel's code. A double
free hands the same frame to two owners, who then overwrite each other.

The PMM keeps a small list of reserved ranges (filled in `pmm_init()`
from the memory map, the kernel image and the multiboot data), so rule 3
is a quick lookup.

## Uses

- `pmm_is_reserved()`, `pmm_is_used()`, `pmm_get_max_blocks()` from core/pmm
- `sec_log()` from the security lab

## Try it

```
seclab verify pmm_free_guard
seclab attack pmm_bad_free live
meminfo                          # used frames unchanged afterwards
```
