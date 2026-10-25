# Attack: heap_overflow

| | |
|---|---|
| **Stopped by** | [`heap_canary`](../../defence/heap_canary/) |
| **Breaks** | consistency: the heap never follows a broken pointer |
| **Severity** | critical |

## What it does

```
[hdr A| data A: 16 bytes ][hdr B: magic size is_free next| data B ]
                          ^-- byte 16 of A is byte 0 of B's header
```

Writing 20 bytes into a 16-byte block overwrites the start of the next
block's header. With more bytes, the attacker controls B's `next`
pointer, and the next heap walk goes wherever they want.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains where A ends and B begins |
| `live` | with interrupts off, really writes 20 bytes into A, shows `heapinfo` reporting CORRUPTED, calls `kfree(A)` so the canary check meets B, then restores B's header exactly |

The heap is never left broken, and nothing else can run while B is
damaged.

## Uses

`kmalloc()`, `kfree()`, `kheap_get_stats()` from core/kheap,
`cpu_irq_save()` from arch/x86/cpu.

## Try it

```
seclab attack heap_overflow live
audit
heapinfo                         # Health: OK
```
