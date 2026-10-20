# Defence: heap_guard

| | |
|---|---|
| **Stops** | [`heap_double_free`](../../attack/heap_double_free/) |
| **Protects** | consistency: the heap's block list stays correct |
| **Called from** | `kfree()` in [core/kheap](../../../core/kheap/kheap.c) |

## The rules

1. `p` is inside the heap, where block data can start
2. `p` is 8-byte aligned (every `kmalloc()` result is)
3. the header just before `p` holds `KHEAP_MAGIC`
4. that block is in use

Rules 1 and 2 only look at the number `p`. So a wild pointer is
refused **before** anything is read through it.

## Why

`kfree(p)` writes to the header 16 bytes before `p`. A pointer from
outside the heap would make it write into random memory. A double free
marks a free block free again; a pointer into the middle of a block
makes it trust random bytes as a header.

**What it cannot catch:** `kfree(p)`, then `q = kmalloc(...)` returns
the same block, then `kfree(p)` again. The block is in use again (by
`q`), so the second free looks valid. Catching that needs to track who
owns each block.

## Uses

- `kheap_header_t`, `KHEAP_MAGIC` from core/kheap
- `KHEAP_START` / `KHEAP_SIZE` from core/layout
- `sec_log()` from the security lab

## Try it

```
seclab verify heap_guard
seclab attack heap_double_free live
heapinfo
```
