# Attack: symlink_loop

| | |
|---|---|
| **Stopped by** | [`link_depth`](defence/link_depth/) |
| **Breaks** | liveness: every path walk ends |
| **Severity** | medium |

## What it does

`ln -s b a ; ln -s a b ; cat a` follows a -> b -> a ... for ever, inside the kernel; a long chain also runs off the kernel stack, since each link is walked by calling the walker again.

## In UzOS

`live` builds `a <-> b` and a chain of 12 links in `/seclab_loop` and `stat()`s each: both end in ELOOP after 8 links.

## Try it

```
seclab attack symlink_loop live
seclab repair
```
