# Attack: smurf_attack

| | |
|---|---|
| **Stopped by** | [`icmp_ignore_broadcast`](defence/icmp_ignore_broadcast/) |
| **Breaks** | resource: one packet causes at most one answer |
| **Severity** | medium |

## What it does

Smurf (1998) turned whole networks into amplifiers with a single forged broadcast ping.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the amplifier |
| `live` | injects echo requests to 10.0.2.255 and 255.255.255.255 |

We do not answer either one.

## Try it

```
seclab attack smurf_attack live
seclab verify icmp_ignore_broadcast

```
