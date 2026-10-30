# Attack: wait_steal

| | |
|---|---|
| **Stopped by** | [`child_only`](defence/child_only/) |
| **Breaks** | privilege: an exit status belongs to the parent |
| **Severity** | medium |

## What it does

An exit status says what a program did, and it is collected **once**.
A stranger who could `wait4()` for your child would read it first, and
you would never see it.

## In UzOS

`live`: the shell (root) starts `backup`, which exits with code 3; the
reaper keeps its record in core/exitlog. User 1's `thief` asks to wait
for it: refused, and the record is still there for the shell.
`proccalls wait` also tries `wait4(1)` from ring 3: refused.

## Try it

```
seclab attack wait_steal live
audit
```
