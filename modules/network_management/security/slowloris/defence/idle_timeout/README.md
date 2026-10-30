# Defence: idle_timeout

| | |
|---|---|
| **Stops** | [`slowloris`](../../) |
| **Protects** | liveness: a slow client cannot hold a server forever |
| **Called from** | `serve()` in [commands/httpd/cmd_httpd.c](../../../../commands/httpd/cmd_httpd.c) |

## The rule

5 s for the whole request, whatever trickles in.

## Why

A timeout per read restarts with every byte.

## Try it

```
seclab verify idle_timeout
seclab attack slowloris live
```
