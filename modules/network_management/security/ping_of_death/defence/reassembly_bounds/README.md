# Defence: reassembly_bounds

| | |
|---|---|
| **Stops** | [`ping_of_death`](../../) |
| **Protects** | integrity: putting fragments together never writes past the buffer |
| **Called from** | `ipv4_reassemble()` in [core/ipv4/fragment.c](../../../../core/ipv4/fragment.c) |

## The rule

offset + length <= the reassembly buffer, computed without overflow.

## Why

Every copy must be bounded by its destination.

## Try it

```
seclab verify reassembly_bounds
seclab attack ping_of_death live
```
