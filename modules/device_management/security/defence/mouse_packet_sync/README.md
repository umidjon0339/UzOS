# Defence: mouse_packet_sync

| | |
|---|---|
| **Stops** | [`mouse_desync`](../../attack/mouse_desync/) |
| **Protects** | consistency: bytes are read as what they really are |
| **Called from** | the packet decoder in [mouse/](../../../mouse/mouse.c) |

## The rules

1. The first byte of every packet has **bit 3 = 1**. If a byte that should
   start a packet does not, drop it and try the next one.
2. If the overflow bits (6, 7) are set, the movement values are garbage:
   drop the whole packet.

## Why

The mouse sends a plain stream of bytes, 3 per packet:

```
flags dx dy | flags dx dy | flags dx dy ...
```

There is no "start of packet" marker. If a byte is lost or an extra one
shows up, the old decoder kept going one byte off:

```
expected:  flags dx dy | flags dx dy
got:       dx dy flags | dx dy flags      <- every packet wrong from now on
```

Movement turns into button clicks and the pointer jumps. The protocol
promises that bit 3 of the flags byte is always 1, so that is how the
decoder finds the start again.

## Uses

- `sec_log()` from the security lab (every dropped byte is evidence)

## Try it

```
seclab attack mouse_desync live
seclab log
```
