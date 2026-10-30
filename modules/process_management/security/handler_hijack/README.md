# Attack: handler_hijack

| | |
|---|---|
| **Stopped by** | [`handler_bounds`](defence/handler_bounds/) |
| **Breaks** | integrity: SIGKILL always works; the kernel decides where its code runs |
| **Severity** | high |

## What it does

`rt_sigaction()` says "when signal N comes, run the code at address X".
A program that catches SIGKILL or ignores SIGSTOP can never be stopped.
A handler address inside the kernel asks the kernel to jump into its own
code whenever the program sends itself a signal.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the four requests |
| `live` | asks `handler_bounds` about each, exactly as `rt_sigaction` does |

`proccalls signals` makes the same requests from a real ring 3 program.

## Try it

```
seclab attack handler_hijack live
proccalls signals
```
