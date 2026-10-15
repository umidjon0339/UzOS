# Defence: file range

**Counters:** [`file_offset_wrap`](../../attack/file_offset_wrap/README.md)

The policy checks an offset against the limit, then compares the length
with the remaining space using subtraction. RAM-disk reads and writes
clamp requests safely and validate the final range before copying.

```text
seclab verify file_range
seclab attack file_offset_wrap live
```
