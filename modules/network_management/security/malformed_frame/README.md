# Attack: malformed_frame

| | |
|---|---|
| **Stopped by** | [`frame_validate`](defence/frame_validate/) |
| **Breaks** | trust: every frame the stack reads is a well-formed Ethernet frame |
| **Severity** | low |

## What it does

Malformed frames test whether a receive path checks lengths before it reads.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the broken frames |
| `live` | injects a 10-byte runt, a 13-byte frame, a 1530-byte giant and an 802.3 frame |
| `repair` | nothing to undo |

With `frame_validate` each one is dropped and counted as an RX error.

## Try it

```
seclab attack malformed_frame live
seclab verify frame_validate
ip -s link        # RX errors grow by 4
```
