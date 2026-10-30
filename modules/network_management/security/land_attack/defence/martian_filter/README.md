# Defence: martian_filter

| | |
|---|---|
| **Stops** | [`land_attack`](../../) |
| **Protects** | trust: a packet's source is a real, other machine |
| **Called from** | `ipv4_receive()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

Source not 0.0.0.0 (except to broadcast), not broadcast, not multicast/class E, not the destination.

## Why

No real machine sends from these addresses.

## Try it

```
seclab verify martian_filter
seclab attack land_attack live
```
