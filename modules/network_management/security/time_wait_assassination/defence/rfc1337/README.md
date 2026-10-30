# Defence: rfc1337

| | |
|---|---|
| **Stops** | [`time_wait_assassination`](../../) |
| **Protects** | integrity: TIME_WAIT lasts its full time |
| **Called from** | `segment()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

RSTs do not end TIME_WAIT.

## Why

RFC 1337; Linux's tcp_rfc1337 sysctl.

## Try it

```
seclab verify rfc1337
seclab attack time_wait_assassination live
```
