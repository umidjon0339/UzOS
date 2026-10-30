# Attack: rst_injection

| | |
|---|---|
| **Stopped by** | [`rst_window_check`](defence/rst_window_check/) |
| **Breaks** | liveness: only the real peer can reset a connection |
| **Severity** | medium |

## What it does

Blind RST injection kills long-lived connections (BGP, SSH) by guessing a sequence number in the window.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the window rule |
| `live` | makes a connection over lo and injects 3 in-window RSTs |

All three get a challenge ACK; the connection lives.

## Try it

```
seclab attack rst_injection live
seclab verify rst_window_check

```
