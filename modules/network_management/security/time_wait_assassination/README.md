# Attack: time_wait_assassination

| | |
|---|---|
| **Stopped by** | [`rfc1337`](defence/rfc1337/) |
| **Breaks** | integrity: TIME_WAIT lasts its full time |
| **Severity** | low |

## What it does

TIME-WAIT assassination (RFC 1337): a RST ends TIME_WAIT, reopening a hazard it exists to close.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains TIME_WAIT |
| `live` | closes a connection over lo and RSTs the client's TIME_WAIT |

The RST is ignored.

## Try it

```
seclab attack time_wait_assassination live
seclab verify rfc1337

```
