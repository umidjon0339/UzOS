# Attack: deep_nesting

| | |
|---|---|
| **Stopped by** | [`depth_limit`](defence/depth_limit/) |
| **Breaks** | liveness: `find /` must not crash the kernel |
| **Severity** | high |

## What it does

`mkdir d; cd d` in a loop is free, but `tree`, `find`, `du` and `rm -r`
call themselves once per level, on a 4 KiB kernel stack. Deep enough,
the next walk runs off the stack.

## In UzOS

`live` nests `d` directories 40 times under `/seclab_deep`: it stops at
32 levels below `/`.

## Try it

```
seclab attack deep_nesting live
seclab repair
```
