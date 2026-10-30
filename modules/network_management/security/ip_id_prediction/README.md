# Attack: ip_id_prediction

| | |
|---|---|
| **Stopped by** | [`random_ip_id`](defence/random_ip_id/) |
| **Breaks** | disclosure: our packets do not reveal how many packets we send |
| **Severity** | low |

## What it does

A global IP id counter leaks how many packets a host sends; the idle scan abuses it to scan a third party anonymously.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the idle scan |
| `live` | sends 5 pings and reads their ids off the tap |

The ids are random; none follows the previous one.

## Try it

```
seclab attack ip_id_prediction live
seclab verify random_ip_id
tcpdump -c 4 icmp & ping -c 4 10.0.2.2
```
