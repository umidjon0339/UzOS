# Attack: ping_of_death

| | |
|---|---|
| **Stopped by** | [`reassembly_bounds`](defence/reassembly_bounds/) |
| **Breaks** | integrity: putting fragments together never writes past the buffer |
| **Severity** | critical |

## What it does

The ping of death (1996): offset + length of the last fragment was never checked against the buffer.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the arithmetic |
| `live` | injects a first fragment, then one at offset 65528 |
| `repair` | drops the half-built datagram |

With `reassembly_bounds` the far fragment is refused and the datagram dropped.

## Try it

```
seclab attack ping_of_death live
seclab verify reassembly_bounds

```
