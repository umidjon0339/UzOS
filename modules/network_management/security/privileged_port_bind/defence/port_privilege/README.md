# Defence: port_privilege

| | |
|---|---|
| **Stops** | [`privileged_port_bind`](../../) |
| **Protects** | privilege: only the administrator runs services on ports below 1024 |
| **Called from** | the bind functions in [core/socket/sock_udp.c](../../../../core/socket/sock_udp.c) and [sock_tcp.c](../../../../core/socket/sock_tcp.c) |

## The rule

Ports 1-1023 need CAP_NET_BIND.

## Why

The Unix rule since BSD: low ports mean "the system".

## Try it

```
seclab verify port_privilege
seclab attack privileged_port_bind live
```
