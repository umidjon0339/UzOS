# Attack: dir_cycle

| | |
|---|---|
| **Stopped by** | [`no_cycle`](defence/no_cycle/) |
| **Breaks** | consistency: the directory tree stays a tree |
| **Severity** | high |

## What it does

```
mv /a /a/b
```

asks for `a` to live inside `b`, but `b` lives inside `a`. Afterwards
`a`'s parent is `b` and `b`'s parent is `a`: a loop that is no longer
connected to `/`. Everything inside is lost, and `pwd` (which walks up
the parents to `/`) never finishes.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | draws the loop it would make |
| `live` | builds `/seclab_a/b`, tries to move `seclab_a` into `b`, and into itself as `seclab_a/inner` (both refused), then removes the directories |

## Uses

`sys_mkdir()`, `sys_rename()`, `sys_rmdir()` from syscalls/.

## Try it

```
seclab attack dir_cycle live
```
