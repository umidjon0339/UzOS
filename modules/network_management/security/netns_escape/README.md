# Attack: netns_escape (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`namespaces`](defence/namespaces/) |
| **Breaks** | isolation: a container's network stays the container's |
| **Severity** | high |

## What it does

Network namespace escapes let a container see or change the host's network.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack netns_escape
seclab verify namespaces

```
