# Attack: amplification_reflection (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`response_limit`](defence/response_limit/) |
| **Breaks** | resource: small questions do not make huge answers to a forged address |
| **Severity** | high |

## What it does

Amplification: servers answer forged questions with much larger answers, all aimed at the victim.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack amplification_reflection
seclab verify response_limit

```
