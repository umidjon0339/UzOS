# Attack: tcp_session_hijack

| | |
|---|---|
| **Stopped by** | [`seq_window_check`](defence/seq_window_check/) |
| **Breaks** | integrity: only the real peer's bytes reach the program |
| **Severity** | high |

## What it does

Blind injection of data into a TCP stream (classic session hijacking).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the window |
| `live` | injects data at rcv_nxt + 20000 and + 10 into a connection over lo |

Neither forgery reaches the program.

## Try it

```
seclab attack tcp_session_hijack live
seclab verify seq_window_check

```
