# Defence: sockaddr_zero

| | |
|---|---|
| **Stops** | [`sockaddr_leak`](../../) |
| **Protects** | disclosure: an address the kernel hands out carries no leftover kernel memory |
| **Called from** | `socket_inet_addr()` in [core/socket/socket.c](../../../../core/socket/socket.c) |

## The rule

Zero the whole structure, then fill it.

## Why

Padding is invisible in the source and visible in the copy.

## Try it

```
seclab verify sockaddr_zero
seclab attack sockaddr_leak live
```
