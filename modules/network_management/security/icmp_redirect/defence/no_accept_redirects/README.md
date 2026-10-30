# Defence: no_accept_redirects

| | |
|---|---|
| **Stops** | [`icmp_redirect`](../../) |
| **Protects** | trust: only the administrator changes the routing table |
| **Called from** | `icmp_receive()` in [core/icmp/icmp.c](../../../../core/icmp/icmp.c) |

## The rule

Redirects are never applied (Linux: `accept_redirects=0`).

## Why

Routing changes need an administrator, not a packet.

## Try it

```
seclab verify no_accept_redirects
seclab attack icmp_redirect live
```
