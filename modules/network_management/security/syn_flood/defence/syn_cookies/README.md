# Defence: syn_cookies

| | |
|---|---|
| **Stops** | [`syn_flood`](../../) |
| **Protects** | liveness: a listening server always takes new connections |
| **Called from** | `new_child()` and the LISTEN path of `tcp_receive()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

A full queue answers with a stateless SYN cookie; a valid cookie in the ACK creates the connection.

## Why

Forged sources never answer, so they cost no memory. Linux: `tcp_syncookies=1`.

## Try it

```
seclab verify syn_cookies
seclab attack syn_flood live
```
