# Defence: rx_desc_check (ILLUSTRATION)

| | |
|---|---|
| **Stops** | [`rx_ring_overflow`](../../) |
| **Protects** | trust: the driver never reads past one receive buffer |
| **Called from** | `receive_all()` in [core/e1000/e1000.c](../../../../core/e1000/e1000.c) |

## The rule

DD and EOP set, no errors, 14 <= length <= buffer size.

## Why

A length that comes from outside is checked like any input.

## Try it

```
seclab verify rx_desc_check
seclab attack rx_ring_overflow live
```
