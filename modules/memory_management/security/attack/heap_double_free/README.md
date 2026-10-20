# Attack: heap_double_free

| | |
|---|---|
| **Stopped by** | [`heap_guard`](../../defence/heap_guard/) |
| **Breaks** | consistency: the heap's block list stays correct |
| **Severity** | high |

## What it does

`kfree(p)` finds the block header 16 bytes **before** `p` and writes to
it. So it goes wrong when `p` is not what `kmalloc()` returned:

| Bad free | What `kfree` would do |
|---|---|
| the same block twice | free an already-free block again |
| `p + 8`, inside a block | treat the block's own data as a header |
| a stack address | write a "header" into the stack |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the three bad frees |
| `live` | allocates two blocks, does the three bad frees for real, frees the blocks properly, then checks the heap is healthy |

## Uses

`kmalloc()`, `kfree()`, `kheap_get_stats()` from core/kheap.

## Try it

```
seclab attack heap_double_free live
heapinfo
```
