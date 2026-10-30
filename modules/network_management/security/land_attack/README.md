# Attack: land_attack

| | |
|---|---|
| **Stopped by** | [`martian_filter`](defence/martian_filter/) |
| **Breaks** | trust: a packet's source is a real, other machine |
| **Severity** | medium |

## What it does

LAND (1997) froze Windows 95 and many routers: a SYN to itself made the stack answer itself in a loop.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists martian sources |
| `live` | injects a LAND SYN (us:80 to us:80) and echoes from 0.0.0.0 and 255.255.255.255 |

With `martian_filter` none reaches TCP or ICMP.

## Try it

```
seclab attack land_attack live
seclab verify martian_filter

```
