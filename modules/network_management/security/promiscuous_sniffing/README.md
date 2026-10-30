# Attack: promiscuous_sniffing

| | |
|---|---|
| **Stopped by** | [`promisc_cap`](defence/promisc_cap/) |
| **Breaks** | disclosure: a user sees only the traffic meant for this machine |
| **Severity** | high |

## What it does

A card in promiscuous mode keeps every frame it sees, not only those
for its own MAC. That is how sniffers (tcpdump, Wireshark) read other
machines' traffic.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows eth0's mode |
| `live` | as uid 1, tries to turn promiscuous mode on |
| `repair` | turns it off |

With `promisc_cap` uid 1 is refused; root's `tcpdump` still works.

## Try it

```
seclab attack promiscuous_sniffing live
seclab verify promisc_cap
tcpdump -c 5      # as root: (promiscuous)
ip link           # PROMISC in the flags while it runs
```
