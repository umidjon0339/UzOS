# Defence: heap_canary

| | |
|---|---|
| **Stops** | [`heap_overflow`](../../attack/heap_overflow/) |
| **Protects** | consistency: the kernel never follows a broken pointer |
| **Called from** | `kmalloc()` and `kfree()` in [core/kheap](../../../core/kheap/kheap.c), for every header |

## The rules

1. the header holds `KHEAP_MAGIC` (the **canary**)
2. `next` is `NULL`, or points further on, still inside the heap

## Why

Heap blocks sit right next to each other:

```
[hdr A| data A (16 bytes) ][hdr B| data B ]...
                         ^ writing 20 bytes into A overwrites hdr B
```

Writing past the end of block A overwrites block B's header, including
its `next` pointer. The next `kmalloc()` that walks the list would
follow that pointer into random memory.

Coal miners carried a canary: if it stopped singing, the air was bad.
The magic number is the heap's canary: if it is gone, the header was
overwritten, so the walk stops. `kmalloc()` returns `NULL` and the event
is logged. That is a clean failure instead of a mysterious crash later.

## Uses

- `kheap_header_t`, `KHEAP_MAGIC` from core/kheap
- `sec_log()` from the security lab

## Try it

```
seclab verify heap_canary
seclab attack heap_overflow live
heapinfo                         # Health: OK again afterwards
```
