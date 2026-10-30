# Attack: arp_spoofing

| | |
|---|---|
| **Stopped by** | [`arp_guard`](defence/arp_guard/) |
| **Breaks** | trust: an IP address belongs to the MAC that really has it |
| **Severity** | high |

## What it does

ARP has no authentication. An attacker on the same link sends "the
gateway 10.0.2.2 is at MY MAC"; the victim stores it and sends all
its internet traffic to the attacker, who forwards it on: a
man-in-the-middle that can read and change everything.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the gateway's ARP entry and what a forgery would change |
| `live` | injects an unasked reply, a gratuitous ARP and a request with a false sender, all claiming the gateway is at `de:ad:be:ef:00:01` |
| `repair` | deletes a poisoned gateway entry |

With `arp_guard` every forgery is refused and the real MAC stays.

## Try it

```
seclab attack arp_spoofing live
seclab verify arp_guard
ip neigh          # the gateway's MAC before and after
audit
```
