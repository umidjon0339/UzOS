# Defence: port_security (CONCEPT)

| | |
|---|---|
| **Stops** | [`mac_flooding`](../../) |
| **Protects** | resource: a switch sends each frame only to the port of its MAC |
| **Called from** | nothing in UzOS: the switch limits how many macs one port may use (port security) |

## The rule

The switch limits how many MACs one port may use (port security).

## Why

The table lives in the switch, so the limit does too.

## Try it

```
seclab verify port_security
seclab attack mac_flooding
```
