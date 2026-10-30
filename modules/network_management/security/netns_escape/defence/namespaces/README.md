# Defence: namespaces (CONCEPT)

| | |
|---|---|
| **Stops** | [`netns_escape`](../../) |
| **Protects** | isolation: a container's network stays the container's |
| **Called from** | nothing in UzOS: separate namespaces, and no network capabilities inside containers |

## The rule

Separate namespaces, and no network capabilities inside containers.

## Why

Isolation is only as strong as the kernel bugs and capabilities allow.

## Try it

```
seclab verify namespaces
seclab attack netns_escape
```
