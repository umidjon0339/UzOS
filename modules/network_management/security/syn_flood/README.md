# Attack: syn_flood

| | |
|---|---|
| **Stopped by** | [`syn_cookies`](defence/syn_cookies/) |
| **Breaks** | liveness: a listening server always takes new connections |
| **Severity** | high |

## What it does

A SYN flood fills a server's queue of half-open connections with forged SYNs; real clients are refused.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains half-open connections |
| `live` | listens on 7777 (queue 4), injects 20 forged SYNs, then connects for real |
| `repair` | closes the listener |

With `syn_cookies` the real client still connects: its SYN-ACK carried a cookie, its ACK proved it real.

## Try it

```
seclab attack syn_flood live
seclab verify syn_cookies
netstat -s       # Tcp: SYNs dropped: listen queue full
```
