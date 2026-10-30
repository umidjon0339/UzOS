# Defence: rmem_limit

| | |
|---|---|
| **Stops** | [`sockbuf_exhaustion`](../../) |
| **Protects** | resource: one socket cannot hold every packet buffer |
| **Called from** | `socket_queue()` in [core/socket/socket.c](../../../../core/socket/socket.c) |

## The rule

Charge a whole buffer per datagram; stop at rcvbuf.

## Why

Memory is what a packet holds, not what it says.

## Try it

```
seclab verify rmem_limit
seclab attack sockbuf_exhaustion live
```
