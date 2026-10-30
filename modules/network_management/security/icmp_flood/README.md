# Attack: icmp_flood

| | |
|---|---|
| **Stopped by** | [`icmp_ratelimit`](defence/icmp_ratelimit/) |
| **Breaks** | resource: answering pings never takes all of the CPU or the link |
| **Severity** | medium |

## What it does

A ping flood makes the victim answer as fast as it can.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the limit |
| `live` | injects 100 echo requests in one burst |

About 10 are answered per 100 ms; the rest are ignored.

## Try it

```
seclab attack icmp_flood live
seclab verify icmp_ratelimit

```
