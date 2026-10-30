# Attack: symlink_race

| | |
|---|---|
| **Stopped by** | [`protected_symlinks`](defence/protected_symlinks/) |
| **Breaks** | trust: root writes where it means to, not where a user points |
| **Severity** | high |

## What it does

A root program writes `/shared/report` every night; the directory is shared. A user plants `report -> /etc/passwd`, and root's next write overwrites a file the user could never write (the classic `/tmp` race).

## In UzOS

`live`: a user 1 process plants `/seclab_shared/report -> /seclab_target`; root opens the report for writing, as its program would: the link is not followed, `/seclab_target` is untouched.

## Try it

```
seclab attack symlink_race live
seclab repair
```
