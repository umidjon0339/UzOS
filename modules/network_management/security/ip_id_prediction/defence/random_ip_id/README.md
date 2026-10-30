# Defence: random_ip_id

| | |
|---|---|
| **Stops** | [`ip_id_prediction`](../../) |
| **Protects** | disclosure: our packets do not reveal how many packets we send |
| **Called from** | `ipv4_send()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

Each packet's id is pseudo-random.

## Why

RFC 6864: the id need only be unique among fragments in flight.

## Try it

```
seclab verify random_ip_id
seclab attack ip_id_prediction live
```
