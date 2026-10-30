# Attack: teardrop

| | |
|---|---|
| **Stopped by** | [`fragment_overlap_check`](defence/fragment_overlap_check/) |
| **Breaks** | integrity: fragments fit together without overlapping |
| **Severity** | high |

## What it does

Teardrop (1997) and its relatives: overlapping fragments crash bad reassembly code or rewrite data after a firewall checked it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | draws the overlap |
| `live` | injects fragment 0-31, then 16-23 |

The overlap is refused and the datagram dropped.

## Try it

```
seclab attack teardrop live
seclab verify fragment_overlap_check

```
