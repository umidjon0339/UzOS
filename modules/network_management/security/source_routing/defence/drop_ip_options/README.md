# Defence: drop_ip_options

| | |
|---|---|
| **Stops** | [`source_routing`](../../) |
| **Protects** | trust: the network, not the sender, decides a packet's path |
| **Called from** | `ipv4_receive()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

LSRR and SSRR options are refused (and broken option lists).

## Why

Paths are the network's decision.

## Try it

```
seclab verify drop_ip_options
seclab attack source_routing live
```
