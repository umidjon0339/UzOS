# Attack: raw_socket_abuse

| | |
|---|---|
| **Stopped by** | [`raw_cap`](defence/raw_cap/) |
| **Breaks** | privilege: a normal user cannot forge whole IP packets |
| **Severity** | high |

## What it does

Raw sockets forge arbitrary packets and sniff a protocol's traffic.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains raw sockets |
| `live` | asks for a raw socket as uid 1, then as root |

uid 1 gets EPERM; root gets the socket.

## Try it

```
seclab attack raw_socket_abuse live
seclab verify raw_cap

```
