# Defence: conn_quota

| | |
|---|---|
| **Stops** | [`connection_exhaustion`](../../) |
| **Protects** | resource: one user cannot take every TCP connection slot |
| **Called from** | `tcp_new()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

8 connections per user (root: CAP_RESOURCE, unlimited).

## Why

Shared slots need a share per user.

## Try it

```
seclab verify conn_quota
seclab attack connection_exhaustion live
```
