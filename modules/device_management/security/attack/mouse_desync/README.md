# Attack: mouse_desync

| | |
|---|---|
| **Stopped by** | [`mouse_packet_sync`](../../defence/mouse_packet_sync/) |
| **Breaks** | consistency: every byte is read as what it is |
| **Severity** | medium |

## What it does

A mouse packet is 3 bytes, but nothing in the stream marks where a
packet starts. The old decoder just counted 0, 1, 2, 0, 1, 2 ... One
lost or extra byte, for example when a key press and a mouse move race
on the shared PS/2 controller, puts it one byte out of step for good:

```
sent :   00 | 09 0A 00 | ...
naive:   [00 09 0A] [00 ...      flags=00 dx=09 dy=0A: wrong click, wrong move
```

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows how a naive decoder misreads the bytes |
| `live` | with interrupts off, feeds the **real** decoder a stray byte, then "left click + move right 10", then a packet with the overflow bit set. Checks the click and move came out right and the overflow was dropped. Puts the pointer back. |

## Uses

`mouse_handle_byte()`, `mouse_get_state()`, `mouse_set_state()` from mouse/.

## Try it

```
seclab attack mouse_desync live
seclab log
```
