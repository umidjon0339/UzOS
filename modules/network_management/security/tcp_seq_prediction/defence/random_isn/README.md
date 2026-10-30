# Defence: random_isn

| | |
|---|---|
| **Stops** | [`tcp_seq_prediction`](../../) |
| **Protects** | trust: nobody can guess the sequence numbers of a connection they cannot see |
| **Called from** | `tcp_connect()` and `new_child()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

ISN = 4 us clock + keyed hash of the connection's addresses and ports.

## Why

RFC 6528; Linux uses SipHash of the 4-tuple plus a clock.

## Try it

```
seclab verify random_isn
seclab attack tcp_seq_prediction live
```
