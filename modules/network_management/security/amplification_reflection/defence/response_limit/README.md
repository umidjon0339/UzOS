# Defence: response_limit (CONCEPT)

| | |
|---|---|
| **Stops** | [`amplification_reflection`](../../) |
| **Protects** | resource: small questions do not make huge answers to a forged address |
| **Called from** | nothing in UzOS: rate-limit responses per client; keep answers small for unverified (udp) clients; bcp 38 stops the forging |

## The rule

Rate-limit responses per client; keep answers small for unverified (UDP) clients; BCP 38 stops the forging.

## Why

The forged source is the whole trick; servers and networks each remove part of it.

## Try it

```
seclab verify response_limit
seclab attack amplification_reflection
```
