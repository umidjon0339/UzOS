# Defence: peercred_check

| | |
|---|---|
| **Stops** | [`unix_impersonation`](../../) |
| **Protects** | identity: a local server knows who its client really is |
| **Called from** | `unix_connect()` and `unix_pair()` in [core/unix/unix.c](../../../../core/unix/unix.c) |

## The rule

The peer's credentials are the connecting socket's owner, recorded by the kernel.

## Why

D-Bus, systemd and polkit all trust SO_PEERCRED for exactly this.

## Try it

```
seclab verify peercred_check
seclab attack unix_impersonation live
```
