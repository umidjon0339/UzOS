# Defence: optlen_check

| | |
|---|---|
| **Stops** | [`sockopt_overflow`](../../) |
| **Protects** | integrity: the kernel reads an option only as far as the caller gave it |
| **Called from** | `socket_setopt()` in [core/socket/socket.c](../../../../core/socket/socket.c) |

## The rule

int options need 4 bytes, timevals 8.

## Why

Every length from user space is checked before it is used.

## Try it

```
seclab verify optlen_check
seclab attack sockopt_overflow live
```
