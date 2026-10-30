# Attack: sack_panic

| | |
|---|---|
| **Stopped by** | [`mss_min`](defence/mss_min/) |
| **Breaks** | resource: the peer cannot make us cut data into absurdly small segments |
| **Severity** | high |

## What it does

CVE-2019-11477 (SACK Panic): a tiny MSS plus SACK overflowed Linux's retransmit queue.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains tiny segments |
| `live` | listens on 7782, injects a SYN with MSS 8 |
| `repair` | closes the listener |

The connection uses MSS 48, not 8.

## Try it

```
seclab attack sack_panic live
seclab verify mss_min

```
