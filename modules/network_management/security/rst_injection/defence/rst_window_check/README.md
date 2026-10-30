# Defence: rst_window_check

| | |
|---|---|
| **Stops** | [`rst_injection`](../../) |
| **Protects** | liveness: only the real peer can reset a connection |
| **Called from** | `segment()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

RST acts only when seq == rcv_nxt; in-window RSTs get a challenge ACK.

## Why

RFC 5961, in Linux since 3.6.

## Try it

```
seclab verify rst_window_check
seclab attack rst_injection live
```
