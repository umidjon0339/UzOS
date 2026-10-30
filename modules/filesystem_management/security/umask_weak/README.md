# Attack: umask_weak

| | |
|---|---|
| **Stopped by** | [`secure_umask`](defence/secure_umask/) |
| **Breaks** | integrity: root's files stay root's to change |
| **Severity** | medium |

## What it does

A program inherits its umask. Start a root program with `umask 0`, and
every file it creates is writable by everyone: a user edits the script
root will run next.

## In UzOS

`live` sets the shell's umask to 0 and creates `/seclab_umask` as root:
its mode is still `rw/r-`. `repair` deletes it.

## Try it

```
seclab attack umask_weak live
umask 0 ; touch x ; stat x     # still 0644
```
