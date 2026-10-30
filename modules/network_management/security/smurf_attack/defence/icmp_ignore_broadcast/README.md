# Defence: icmp_ignore_broadcast

| | |
|---|---|
| **Stops** | [`smurf_attack`](../../) |
| **Protects** | resource: one packet causes at most one answer |
| **Called from** | `icmp_receive()` in [core/icmp/icmp.c](../../../../core/icmp/icmp.c) |

## The rule

No echo reply to a request sent to a broadcast address.

## Why

One packet must never cause many.

## Try it

```
seclab verify icmp_ignore_broadcast
seclab attack smurf_attack live
```
