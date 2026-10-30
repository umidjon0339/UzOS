# Defence: tcp_flag_sanity

| | |
|---|---|
| **Stops** | [`xmas_null_scan`](../../) |
| **Protects** | disclosure: nonsense segments tell an attacker nothing |
| **Called from** | `tcp_receive()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

No answer to flag combinations no real stack sends.

## Why

Silence gives a scanner nothing to read.

## Try it

```
seclab verify tcp_flag_sanity
seclab attack xmas_null_scan live
```
