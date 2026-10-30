# Attack: icmp_redirect

| | |
|---|---|
| **Stopped by** | [`no_accept_redirects`](defence/no_accept_redirects/) |
| **Breaks** | trust: only the administrator changes the routing table |
| **Severity** | high |

## What it does

ICMP redirects let any host on the path rewrite our routes.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains redirects |
| `live` | injects a redirect for 8.8.8.8 via 10.0.2.66 |
| `repair` | deletes a host route if one was added |

The redirect is ignored; `ip route get 8.8.8.8` is unchanged.

## Try it

```
seclab attack icmp_redirect live
seclab verify no_accept_redirects
ip route get 8.8.8.8
```
