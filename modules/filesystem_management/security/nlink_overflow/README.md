# Attack: nlink_overflow

| | |
|---|---|
| **Stopped by** | [`nlink_bounds`](defence/nlink_bounds/) |
| **Breaks** | consistency: nlink says truly how many names a file has |
| **Severity** | medium |

## What it does

`nlink` decides when a file is freed. A counter pushed up without end wraps: with 16-bit counts, 65536 links read as 0, and the next unlink frees a file 65535 names still point at.

## In UzOS

`live` makes 24 hard links to one file with the real `link()`: 15 work (16 names), the rest fail with EMLINK.

## Try it

```
seclab attack nlink_overflow live
seclab repair
```
