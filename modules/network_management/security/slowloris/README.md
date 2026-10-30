# Attack: slowloris

| | |
|---|---|
| **Stopped by** | [`idle_timeout`](defence/idle_timeout/) |
| **Breaks** | liveness: a slow client cannot hold a server forever |
| **Severity** | medium |

## What it does

Slowloris (2009) keeps a server's workers busy with requests that never end.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the trick |
| `live` | needs `httpd start`: sends a request one byte every 2 s |

httpd drops the slow client after 5 s.

## Try it

```
seclab attack slowloris live
seclab verify idle_timeout
httpd start
seclab attack slowloris live
httpd status
```
