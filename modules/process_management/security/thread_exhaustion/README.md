# Attack: thread_exhaustion

| | |
|---|---|
| **Stopped by** | [`thread_quota`](defence/thread_quota/) |
| **Breaks** | resource: 64 process slots for the whole system |
| **Severity** | high |

## What it does

Every thread is a process slot and a kernel stack. A program that starts
threads in a loop fills the table. `slot_quota` stops a normal user at
8 processes, but a program run by root is exempt from it.

## In UzOS

[thread_exhaustion.s](thread_exhaustion.s) (ring 3, uid 1) calls
`clone(CLONE_THREAD)` 10 times: 3 threads start (4 with the main one),
the rest are refused. `exit_group()` then ends them all.

## Try it

```
seclab attack thread_exhaustion live
```
