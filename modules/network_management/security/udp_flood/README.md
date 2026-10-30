# Attack: udp_flood

| | |
|---|---|
| **Stopped by** | [`udp_ratelimit`](defence/udp_ratelimit/) |
| **Breaks** | resource: datagrams to closed ports do not make us flood their sender |
| **Severity** | medium |

## What it does

A UDP flood to closed ports makes the target answer every datagram, and a forged source makes it a reflector.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the reflection |
| `live` | injects 50 datagrams from 10.0.2.89 to closed ports |

Only 5 answers reach the source; the rest are silent.

## Try it

```
seclab attack udp_flood live
seclab verify udp_ratelimit
netstat -s       # Udp: ... to unknown ports
```
