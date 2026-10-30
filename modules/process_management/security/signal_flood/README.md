# Attack: signal_flood

| | |
|---|---|
| **Stopped by** | [`sigqueue_limit`](defence/sigqueue_limit/) |
| **Breaks** | resource: the queued-signal pool is shared by everyone |
| **Severity** | medium |

## What it does

`rt_sigqueueinfo()` sends a signal with a value. The kernel must keep
every queued value until it is delivered. A target that blocks the
signal never takes them, so a user who keeps queueing fills the pool,
and no one else can queue a signal any more.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the 64-entry pool and what one user could take |
| `live` | as uid 1, starts `flood_target` (ring 0: signals stay queued) and queues 64 signals to it |
| `repair` | kills `flood_target`; the reaper frees its entries |

With `sigqueue_limit` on, 16 are queued, then every try is refused.

## Uses

`sigqueue_add()`, `sigqueue_count_user()` from core/signal,
`process_create()` / `process_kill()` from core/process.

## Try it

```
seclab attack signal_flood live
audit
seclab repair
ulimit -i          # the user's share: RLIMIT_SIGPENDING
```
