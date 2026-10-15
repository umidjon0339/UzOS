# Attack: file offset wrap

**Breaks:** `consistency` · **Severity:** HIGH · **Paired defence:** [`file_range`](../../defence/file_range/README.md)

An offset plus length can wrap around and bypass the RAM-disk file-size
limit. The demo probes the range policy only; it does not access or
modify file contents.

```text
seclab attack file_offset_wrap
seclab attack file_offset_wrap live
seclab verify file_range
```
