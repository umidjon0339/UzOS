# Defence: udp_ratelimit

| | |
|---|---|
| **Stops** | [`udp_flood`](../../) |
| **Protects** | resource: datagrams to closed ports do not make us flood their sender |
| **Called from** | `udp_receive()` in [core/udp/udp.c](../../../../core/udp/udp.c) |

## The rule

At most 5 'port unreachable' per second to one source.

## Why

Answers must never cost more than the flood.

## Try it

```
seclab verify udp_ratelimit
seclab attack udp_flood live
```
