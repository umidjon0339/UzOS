# Defence: rp_filter

| | |
|---|---|
| **Stops** | [`ip_spoofing`](../../) |
| **Protects** | trust: a packet's source address is where it really came from |
| **Called from** | `ipv4_receive()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

The source must route back out of the same interface (strict reverse path).

## Why

Linux's `rp_filter=1` does this; BCP 38 asks networks to filter the same way.

## Try it

```
seclab verify rp_filter
seclab attack ip_spoofing live
```
