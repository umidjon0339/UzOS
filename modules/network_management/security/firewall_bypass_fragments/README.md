# Attack: firewall_bypass_fragments

| | |
|---|---|
| **Stopped by** | [`reassemble_before_filter`](defence/reassemble_before_filter/) |
| **Breaks** | trust: a firewall rule sees every packet it is meant to stop |
| **Severity** | high |

## What it does

Tiny / overlapping fragment attacks split headers so that a per-fragment filter never sees what its rule needs.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the tiny fragment attack |
| `live` | adds a SYN-drop rule for 7791, injects the SYN in two fragments |
| `repair` | removes the rule, closes the listener |

The reassembled SYN meets the rule and is dropped.

## Try it

```
seclab attack firewall_bypass_fragments live
seclab verify reassemble_before_filter
iptables -L -v
```
