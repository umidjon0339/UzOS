# Attack: dns_spoofing

| | |
|---|---|
| **Stopped by** | [`dns_txid_port_random`](defence/dns_txid_port_random/) |
| **Breaks** | trust: only the real DNS server's answer is believed |
| **Severity** | high |

## What it does

DNS cache poisoning: a forged answer that guesses the query's id (and port) arrives before the real one.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the guess |
| `live` | asks 3 DNS questions and reads their ids and ports off the tap |

Ports and ids jump randomly; a forger must guess 32 bits.

## Try it

```
seclab attack dns_spoofing live
seclab verify dns_txid_port_random

```
