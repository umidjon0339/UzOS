# Defence: scan_detect

| | |
|---|---|
| **Stops** | [`port_scan`](../../) |
| **Protects** | disclosure: a stranger cannot cheaply map every closed port |
| **Called from** | `refuse()` in [core/tcp/tcp.c](../../../../core/tcp/tcp.c) |

## The rule

10 closed ports per second from one source: 10 s of silence for it.

## Why

Make mapping slow and noisy (portsentry, psad, fail2ban do the same in user space).

## Try it

```
seclab verify scan_detect
seclab attack port_scan live
```
