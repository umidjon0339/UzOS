# Attack: zero_window

| | |
|---|---|
| **Stopped by** | [`persist_timeout`](defence/persist_timeout/) |
| **Breaks** | resource: a peer cannot hold our send buffer forever |
| **Severity** | medium |

## What it does

Sockstress (2008): peers that hold a zero window pin the sender's buffers and connections forever.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the stalled window |
| `live` | a connection over lo whose receiver never reads; waits 7 s |

After 4 probes the sender closes the connection (ETIMEDOUT).

## Try it

```
seclab attack zero_window live
seclab verify persist_timeout

```
