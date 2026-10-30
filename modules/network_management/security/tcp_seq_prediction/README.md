# Attack: tcp_seq_prediction

| | |
|---|---|
| **Stopped by** | [`random_isn`](defence/random_isn/) |
| **Breaks** | trust: nobody can guess the sequence numbers of a connection they cannot see |
| **Severity** | high |

## What it does

Predictable ISNs let a blind attacker forge a TCP connection from a trusted address (Mitnick vs. Shimomura, 1994).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains ISN guessing |
| `live` | opens 4 connections over lo and compares the server's ISNs |

The ISNs differ by random-looking amounts.

## Try it

```
seclab attack tcp_seq_prediction live
seclab verify random_isn

```
