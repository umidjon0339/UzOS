# Attack: fork_bomb

| | |
|---|---|
| **Stopped by** | [`slot_quota`](../../defence/slot_quota/) |
| **Breaks** | resource: there are only 64 process slots |
| **Severity** | critical |

## What it does

A fork bomb creates processes in a loop. In a real one every child
loops too, so the number doubles until the process table is full. Then
nothing else can start, not even a shell to stop it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | counts the free slots a bomb would take; changes nothing |
| `live` | as uid 1, creates up to 16 sleeping `bomb` processes |
| `repair` | kills every `bomb` process |

With `slot_quota` on, the 9th creation is refused: a user may own 8.

## Uses

`process_create()`, `process_kill()`, `process_count_free_slots()` from core/process.

## Try it

```
seclab attack fork_bomb          # simulate
seclab attack fork_bomb live
ps
seclab repair
```
