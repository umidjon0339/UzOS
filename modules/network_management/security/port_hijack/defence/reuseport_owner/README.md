# Defence: reuseport_owner

| | |
|---|---|
| **Stops** | [`port_hijack`](../../) |
| **Protects** | isolation: another user cannot share (and steal from) my port |
| **Called from** | `udp_bind()` in [core/udp/udp.c](../../../../core/udp/udp.c) |

## The rule

Sharing needs SO_REUSEPORT on both sockets and the same owner.

## Why

Linux's rule since SO_REUSEPORT appeared (3.9).

## Try it

```
seclab verify reuseport_owner
seclab attack port_hijack live
```
