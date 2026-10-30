# Attack: source_routing

| | |
|---|---|
| **Stopped by** | [`drop_ip_options`](defence/drop_ip_options/) |
| **Breaks** | trust: the network, not the sender, decides a packet's path |
| **Severity** | medium |

## What it does

Source routing lets the sender choose the path, around firewalls and back through itself.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains LSRR |
| `live` | injects an echo request with an LSRR option |

The packet is dropped before ICMP sees it.

## Try it

```
seclab attack source_routing live
seclab verify drop_ip_options

```
