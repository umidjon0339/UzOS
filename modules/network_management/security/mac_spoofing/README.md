# Attack: mac_spoofing

| | |
|---|---|
| **Stopped by** | [`mac_lock`](defence/mac_lock/) |
| **Breaks** | privilege: only the administrator decides the card's MAC address |
| **Severity** | medium |

## What it does

Setting another machine's MAC lets you receive its frames on a switch
and pass MAC-based access control. On Linux only root may do it
(`ip link set eth0 address ...`).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows eth0's MAC |
| `live` | as uid 1, tries `netdev_set_mac(eth0, 02:de:ad:00:00:01)` |
| `repair` | puts the card's own MAC back |

With `mac_lock` the change is refused for uid 1.

## Try it

```
seclab attack mac_spoofing live
seclab verify mac_lock
ip link set eth0 address 02:00:00:00:00:01    # as root: allowed
ip link
```
