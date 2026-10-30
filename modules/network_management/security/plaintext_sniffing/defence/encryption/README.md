# Defence: encryption (ILLUSTRATION)

| | |
|---|---|
| **Stops** | [`plaintext_sniffing`](../../) |
| **Protects** | disclosure: a password travels unreadable to anyone on the path |
| **Called from** | nothing in UzOS: UzOS has no TLS or SSH |

## The rule

Secrets only inside TLS or SSH.

## Why

Encryption is the only defence against an eavesdropper on the path.

## Try it

```
seclab verify encryption
seclab attack plaintext_sniffing live
```
