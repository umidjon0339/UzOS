# Defence: arp_guard

| | |
|---|---|
| **Stops** | [`arp_spoofing`](../../) |
| **Protects** | trust: an IP address belongs to the MAC that really has it |
| **Called from** | `arp_receive()` in [core/arp/arp.c](../../../../core/arp/arp.c) |

## The rule

| The sender's entry | Allowed to set its MAC |
|---|---|
| none | only a request asking for **our** address |
| INCOMPLETE (we asked) | yes: it is the answer |
| REACHABLE | only the same MAC (a refresh) |
| PERMANENT (`arp -s`) | never |

## Why

Without proof of identity, the safe rule is: never let a packet change
what we already know. A real card swap is learned when the entry
expires (60 s). Switches add Dynamic ARP Inspection; hosts can pin
entries with `arp -s`.

## Try it

```
seclab verify arp_guard
seclab attack arp_spoofing live
```
