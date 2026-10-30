# Defence: persist_timeout

| | |
|---|---|
| **Stops** | [`zero_window`](../../) |
| **Protects** | resource: a peer cannot hold our send buffer forever |
| **Called from** | `retransmit()` in [core/tcp/tcp_timer.c](../../../../core/tcp/tcp_timer.c) |

## The rule

At most 4 window probes; then the connection ends.

## Why

A peer that never opens its window is not talking.

## Try it

```
seclab verify persist_timeout
seclab attack zero_window live
```
