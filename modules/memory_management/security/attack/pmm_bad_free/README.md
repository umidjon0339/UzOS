# Attack: pmm_bad_free

| | |
|---|---|
| **Stopped by** | [`pmm_free_guard`](../../defence/pmm_free_guard/) |
| **Breaks** | consistency: each frame has exactly one owner |
| **Severity** | high |

## What it does

The PMM gives the most recently freed frames to the next caller. So:

- freeing the **kernel's own frame** hands kernel code to the next
  process stack, which then overwrites it
- freeing a **misaligned** or **non-RAM** address flips bits in the
  bitmap that describe some other frame
- a **double free** hands one frame to two owners

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the four bad frees |
| `live` | calls the real `pmm_free_block()` with each one (including a real double free) and checks the used-frame count does not move |

## Uses

`pmm_alloc_block()` / `pmm_free_block()` from core/pmm,
`_kernel_start` from boot/linker.ld.

## Try it

```
seclab attack pmm_bad_free live
seclab log
```
