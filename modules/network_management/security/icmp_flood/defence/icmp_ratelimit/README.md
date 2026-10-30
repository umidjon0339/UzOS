# Defence: icmp_ratelimit

| | |
|---|---|
| **Stops** | [`icmp_flood`](../../) |
| **Protects** | resource: answering pings never takes all of the CPU or the link |
| **Called from** | `icmp_receive()` and `icmp_send_error()` in [core/icmp/icmp.c](../../../../core/icmp/icmp.c) |

## The rule

At most 10 ICMP messages per 100 ms (a token bucket).

## Why

An answer should never cost more than the question.

## Try it

```
seclab verify icmp_ratelimit
seclab attack icmp_flood live
```
