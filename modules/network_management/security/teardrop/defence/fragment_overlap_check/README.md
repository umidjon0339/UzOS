# Defence: fragment_overlap_check

| | |
|---|---|
| **Stops** | [`teardrop`](../../) |
| **Protects** | integrity: fragments fit together without overlapping |
| **Called from** | `ipv4_reassemble()` in [core/ipv4/fragment.c](../../../../core/ipv4/fragment.c) |

## The rule

A fragment may not cover a block that is already filled.

## Why

RFC 5722 and Linux drop overlapping fragments.

## Try it

```
seclab verify fragment_overlap_check
seclab attack teardrop live
```
