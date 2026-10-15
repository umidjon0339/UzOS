# Attack: PMM out-of-range free

**Breaks:** `consistency` · **Severity:** HIGH · **Paired defence:** [`pmm_free_bounds`](../../defence/pmm_free_bounds/README.md)

An unchecked frame address can select a bitmap index outside the range
described by the memory map, corrupting allocator state. The live demo
passes an invalid address to the guarded free operation and confirms the
used-frame count does not change.

```text
seclab attack pmm_free_oob
seclab attack pmm_free_oob live
seclab verify pmm_free_bounds
```
