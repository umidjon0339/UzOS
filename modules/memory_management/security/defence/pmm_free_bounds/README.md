# Defence: PMM free bounds

**Counters:** [`pmm_free_oob`](../../attack/pmm_free_oob/README.md)

Before indexing the bitmap, PMM release operations check that the
address is non-null, page-aligned, and that the full frame range lies
within physical memory. The range check avoids adding the frame count to
the starting index, so it cannot wrap.

```text
seclab verify pmm_free_bounds
seclab attack pmm_free_oob live
```
