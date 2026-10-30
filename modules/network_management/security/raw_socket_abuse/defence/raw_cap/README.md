# Defence: raw_cap

| | |
|---|---|
| **Stops** | [`raw_socket_abuse`](../../) |
| **Protects** | privilege: a normal user cannot forge whole IP packets |
| **Called from** | `socket_create()` in [core/socket/socket.c](../../../../core/socket/socket.c) |

## The rule

SOCK_RAW needs CAP_NET_RAW.

## Why

Same as Linux; ping and nmap -sS need root (or the capability).

## Try it

```
seclab verify raw_cap
seclab attack raw_socket_abuse live
```
