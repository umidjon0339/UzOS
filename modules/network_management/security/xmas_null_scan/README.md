# Attack: xmas_null_scan

| | |
|---|---|
| **Stopped by** | [`tcp_flag_sanity`](defence/tcp_flag_sanity/) |
| **Breaks** | disclosure: nonsense segments tell an attacker nothing |
| **Severity** | low |

## What it does

NULL/Xmas/FIN scans map ports through the RFC 793 rule that closed ports answer nonsense with RST.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the scans |
| `live` | injects NULL, Xmas and SYN+FIN segments to closed port 9 |

All three are dropped silently.

## Try it

```
seclab attack xmas_null_scan live
seclab verify tcp_flag_sanity

```
