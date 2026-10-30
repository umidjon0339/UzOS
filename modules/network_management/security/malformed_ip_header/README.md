# Attack: malformed_ip_header

| | |
|---|---|
| **Stopped by** | [`ip_header_validate`](defence/ip_header_validate/) |
| **Breaks** | trust: the IP header's lengths fit the packet that carries them |
| **Severity** | medium |

## What it does

Header fields that lie lead a trusting parser out of its buffer.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the lies |
| `live` | injects IHL 3, version 6, total length 4000 and a bad checksum |

All four are dropped (`netstat -s`: packets with invalid headers).

## Try it

```
seclab attack malformed_ip_header live
seclab verify ip_header_validate

```
