# Attack: timestamp_forgery

| | |
|---|---|
| **Stopped by** | [`time_owner`](defence/time_owner/) |
| **Breaks** | integrity: a file's times are evidence |
| **Severity** | medium |

## What it does

A user who may write a shared root file edits it, then sets its mtime
back a year: nobody sees the change, backup tools skip the file.

## In UzOS

`live`: root's `/seclab_evidence` is writable by others. A process of
user 1 calls the real `utimensat()`: a chosen time is refused, "now" is
allowed (writing would have set it anyway).

## Try it

```
seclab attack timestamp_forgery live
touch -r /hello notes      # chosen times: owner only
```
