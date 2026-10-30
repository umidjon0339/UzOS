# Attack: unix_impersonation

| | |
|---|---|
| **Stopped by** | [`peercred_check`](defence/peercred_check/) |
| **Breaks** | identity: a local server knows who its client really is |
| **Severity** | high |

## What it does

A local client lies about who it is to a privileged service.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains SO_PEERCRED |
| `live` | a uid-1 client tells a UNIX server it is root; the server asks SO_PEERCRED |

SO_PEERCRED reports uid 1, whatever the client said.

## Try it

```
seclab attack unix_impersonation live
seclab verify peercred_check

```
