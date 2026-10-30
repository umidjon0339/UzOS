# Attack: port_hijack

| | |
|---|---|
| **Stopped by** | [`reuseport_owner`](defence/reuseport_owner/) |
| **Breaks** | isolation: another user cannot share (and steal from) my port |
| **Severity** | medium |

## What it does

Port hijacking through SO_REUSEPORT: a stranger's socket joins a server's port and receives part of its traffic.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains SO_REUSEPORT |
| `live` | uid 0 binds UDP 5353 with SO_REUSEPORT; uid 1 tries to join |

uid 1 is refused; the same user's sockets could share it.

## Try it

```
seclab attack port_hijack live
seclab verify reuseport_owner

```
