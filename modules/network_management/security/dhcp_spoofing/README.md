# Attack: dhcp_spoofing

| | |
|---|---|
| **Stopped by** | [`dhcp_validate`](defence/dhcp_validate/) |
| **Breaks** | trust: our address, router and DNS come from the real DHCP server |
| **Severity** | high |

## What it does

A rogue DHCP server hands out itself as the router and DNS server: every packet and every name lookup goes through it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the race |
| `live` | restarts DHCP and injects a rogue OFFER at once |
| `repair` | asks DHCP again if the rogue won |

The rogue offer is refused; the lease comes from 10.0.2.2 again.

## Try it

```
seclab attack dhcp_spoofing live
seclab verify dhcp_validate
dhclient -l
```
