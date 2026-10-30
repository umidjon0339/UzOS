# Defence: nlink_bounds

| | |
|---|---|
| **Stops** | [`nlink_overflow`](../../) |
| **Protects** | consistency: nlink says truly how many names a file has |
| **Called from** | `ramdisk_link()` in core/ramdisk, before nlink counts up |

## The rule

A file has at most **16** names (`FS_LINK_MAX`); a ceiling far below the counter's limit makes the wrap impossible.

## Try it

```
seclab verify nlink_bounds
```
