# Attack: mmap length wrap

**Breaks:** `resource` · **Severity:** MEDIUM · **Paired defence:** [`mmap_page_count`](../../defence/mmap_page_count/README.md)

Rounding a byte length with `length + PAGE_SIZE - 1` can overflow before
division and produce an incorrect page count. The attack checks the
largest `size_t` request against the safe page-count policy. It allocates
no memory.

```text
seclab attack mmap_length_wrap
seclab attack mmap_length_wrap live
seclab verify mmap_page_count
```
