# Attack: fragment_flood

| | |
|---|---|
| **Stopped by** | [`frag_mem_limit`](defence/frag_mem_limit/) |
| **Breaks** | resource: the reassembly slots are shared fairly |
| **Severity** | medium |

## What it does

Fragments that never complete hold reassembly memory until they time out; a flood of them starves everyone else.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the slots |
| `live` | injects first fragments of 4 datagrams from one source |
| `repair` | frees the slots |

One source gets 2 slots; the others stay free.

## Try it

```
seclab attack fragment_flood live
seclab verify frag_mem_limit

```
