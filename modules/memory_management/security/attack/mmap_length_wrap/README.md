# Attack: mmap_length_wrap

| | |
|---|---|
| **Stopped by** | [`mmap_page_count`](../../defence/mmap_page_count/) |
| **Breaks** | resource: every mapped byte has a page behind it |
| **Severity** | medium |

## What it does

`mmap(length)` must turn bytes into whole pages. The usual formula is

```c
pages = (length + 4095) / 4096;
```

With `length = 0xFFFFFFFF` the addition wraps to `0xFFE`, so
`pages = 0`. mmap then returns an address with **no** memory behind it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the naive formula giving 0 pages |
| `live` | calls the real `sys_mmap()` with `0xFFFFFFFF`, `0` and 16 MiB + 1 (all must fail), then with 4096 (must work) |

## Uses

`sys_mmap()` / `sys_munmap()` from syscalls/.

## Try it

```
seclab attack mmap_length_wrap
seclab attack mmap_length_wrap live
```
