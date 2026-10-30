# Defence: dhcp_snooping (CONCEPT)

| | |
|---|---|
| **Stops** | [`dhcp_starvation`](../../) |
| **Protects** | resource: the DHCP server has addresses for every real machine |
| **Called from** | nothing in UzOS: the switch rations dhcp per port (snooping, rate limits) |

## The rule

The switch rations DHCP per port (snooping, rate limits).

## Why

A server cannot tell fake clients apart; the port they come from can.

## Try it

```
seclab verify dhcp_snooping
seclab attack dhcp_starvation
```
