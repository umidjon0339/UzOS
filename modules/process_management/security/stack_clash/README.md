# Attack: stack_clash

| | |
|---|---|
| **Stopped by** | [`guard_gap`](defence/guard_gap/) |
| **Breaks** | isolation: a stack must fault before it overwrites anything |
| **Severity** | medium |

## What it does

A stack grows down without asking. If memory is mapped right below it,
a deep recursion or a big local array writes into that memory with no
fault: the "Stack Clash" (Qualys, 2017).

## In UzOS

`live`: (1) a program header whose segment lies in the execve stack
area, checked as `execve` checks it; (2) a page mapped right below the
last slot's stack, with the gap checked as `execve` does. Both refused.

## Try it

```
seclab attack stack_clash live
```
