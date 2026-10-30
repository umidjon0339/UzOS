# Attack: ip_spoofing

| | |
|---|---|
| **Stopped by** | [`rp_filter`](defence/rp_filter/) |
| **Breaks** | trust: a packet's source address is where it really came from |
| **Severity** | high |

## What it does

Spoofed sources let an attacker pose as a trusted host (localhost, a server on the LAN) or hide where an attack comes from.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the reverse path test |
| `live` | injects echo requests on eth0 from 127.0.0.1 and from our own address |

With `rp_filter` both are dropped before ICMP sees them.

## Try it

```
seclab attack ip_spoofing live
seclab verify rp_filter
netstat -s       # Ip: ... invalid addresses
```
