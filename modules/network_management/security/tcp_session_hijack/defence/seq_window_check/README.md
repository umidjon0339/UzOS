# Defence: seq_window_check

| | |
|---|---|
| **Stops** | [`tcp_session_hijack`](../../) |
| **Protects** | integrity: only the real peer's bytes reach the program |
| **Called from** | `segment()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

A segment must touch [rcv_nxt, rcv_nxt + window); data must start at rcv_nxt to be delivered.

## Why

Together with random ISNs, this is what keeps a blind attacker out.

## Try it

```
seclab verify seq_window_check
seclab attack tcp_session_hijack live
```
