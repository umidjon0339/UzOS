# Defence: mss_min

| | |
|---|---|
| **Stops** | [`sack_panic`](../../) |
| **Protects** | resource: the peer cannot make us cut data into absurdly small segments |
| **Called from** | `new_child()` and `syn_sent()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

MSS = max(announced, 48).

## Why

Linux's fix for CVE-2019-11477/11479.

## Try it

```
seclab verify mss_min
seclab attack sack_panic live
```
