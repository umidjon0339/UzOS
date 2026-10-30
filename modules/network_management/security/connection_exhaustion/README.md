# Attack: connection_exhaustion

| | |
|---|---|
| **Stopped by** | [`conn_quota`](defence/conn_quota/) |
| **Breaks** | resource: one user cannot take every TCP connection slot |
| **Severity** | medium |

## What it does

Connection-slot exhaustion: one user keeps every connection, so nobody else can connect.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the slots |
| `live` | as uid 1, asks TCP for 16 connections |
| `repair` | frees them |

uid 1 gets 8; the rest stay free.

## Try it

```
seclab attack connection_exhaustion live
seclab verify conn_quota

```
