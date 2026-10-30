# Attack: port_scan

| | |
|---|---|
| **Stopped by** | [`scan_detect`](defence/scan_detect/) |
| **Breaks** | disclosure: a stranger cannot cheaply map every closed port |
| **Severity** | low |

## What it does

Port scans (nmap) learn which services run from the RSTs and SYN-ACKs they get back.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the threshold |
| `live` | injects SYNs from 10.0.2.91 to ports 1000-1039 |

After 10 RSTs the source is noticed and gets silence.

## Try it

```
seclab attack port_scan live
seclab verify scan_detect

```
