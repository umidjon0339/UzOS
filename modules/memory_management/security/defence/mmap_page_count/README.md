# Defence: mmap page count

**Counters:** [`mmap_length_wrap`](../../attack/mmap_length_wrap/README.md)

`mem_mmap_page_count()` divides first and adds one only when a remainder
exists. This calculates the ceiling without overflowing. `sys_mmap()`
and `sys_munmap()` share the helper, and the selftest covers zero,
page-boundary, and largest-size inputs.

```text
seclab verify mmap_page_count
seclab attack mmap_length_wrap live
```
