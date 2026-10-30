# Attack: rx_ring_overflow (ILLUSTRATION)

| | |
|---|---|
| **Stopped by** | [`rx_desc_check`](defence/rx_desc_check/) |
| **Breaks** | trust: the driver never reads past one receive buffer |
| **Severity** | medium |

## What it does

A device writes the length of each received frame. A driver that
trusts it copies past its buffer when the device lies (a bug, or a
malicious device: Thunderbolt and FireWire attacks).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the receive descriptor |
| `live` | runs three forged descriptors through the driver's check |
| `repair` | nothing |

All three forged descriptors are refused.

## Try it

```
seclab attack rx_ring_overflow live
seclab verify rx_desc_check
ethtool -S       # rx_errors
```
