# Attack: env_injection

| | |
|---|---|
| **Stopped by** | [`env_sanitize`](defence/env_sanitize/) |
| **Breaks** | trust: a program decides what code it runs |
| **Severity** | high |

## What it does

The environment is input a program did not choose. `LD_PRELOAD` loads
attacker code before `main()`; `IFS` and `BASH_ENV` change what a shell
script does; an escape sequence repaints a terminal.

## In UzOS

`live` hands each entry to `env_sanitize` as `execve` does: the harmful
ones are dropped, `HOME=` passes. `proccalls exec` runs the real
program (`showargs`) with them: it sees one variable, `HOME`.

## Try it

```
seclab attack env_injection live
proccalls exec
```
