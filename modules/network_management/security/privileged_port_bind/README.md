# Attack: privileged_port_bind

| | |
|---|---|
| **Stopped by** | [`port_privilege`](defence/port_privilege/) |
| **Breaks** | privilege: only the administrator runs services on ports below 1024 |
| **Severity** | medium |

## What it does

A user who binds a well-known port impersonates the system's service.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains privileged ports |
| `live` | as uid 1, binds TCP 80 and UDP 53 |

Both binds fail with EACCES.

## Try it

```
seclab attack privileged_port_bind live
seclab verify port_privilege

```
