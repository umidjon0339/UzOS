# Defence: frame_validate

| | |
|---|---|
| **Stops** | [`malformed_frame`](../../) |
| **Protects** | trust: every frame the stack reads is a well-formed Ethernet frame |
| **Called from** | `ethernet_receive()` in [core/ethernet/ethernet.c](../../../../core/ethernet/ethernet.c) |

## The rule

14 to 1514 bytes, and a type field of at least 0x0600.

## Why

Check the length before every read.

## Try it

```
seclab verify frame_validate
seclab attack malformed_frame live
```
