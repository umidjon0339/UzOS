# Attack: mac_flooding (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`port_security`](defence/port_security/) |
| **Breaks** | resource: a switch sends each frame only to the port of its MAC |
| **Severity** | medium |

## What it does

Thousands of fake source MACs fill a switch's MAC table; it then floods all traffic to every port (fail-open), and a sniffer on any port sees everything.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack mac_flooding
seclab verify port_security

```
