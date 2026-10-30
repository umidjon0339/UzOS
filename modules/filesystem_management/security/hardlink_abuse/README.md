# Attack: hardlink_abuse

| | |
|---|---|
| **Stopped by** | [`protected_hardlinks`](defence/protected_hardlinks/) |
| **Breaks** | privilege: a user decides nothing about root's files |
| **Severity** | medium |

## What it does

A hard link is a second name for the same file. A user who links root's file into their home keeps it alive after root deletes it, and can wait for root to `chown -R` their home - giving root's file away.

## In UzOS

`live`: a user 1 process calls the real `link("/seclab_secret", "/home/guest/mine")`: refused.

## Try it

```
seclab attack hardlink_abuse live
seclab repair
```
