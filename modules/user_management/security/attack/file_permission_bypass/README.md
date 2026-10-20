# Attack: file_permission_bypass

| | |
|---|---|
| **Stopped by** | [`file_access`](../../defence/file_access/) |
| **Breaks** | isolation: a user's files are theirs |
| **Severity** | high |

## What it does

As root, the attack makes two files: `/home/root/secret` (mode 60) and
`/seclab_root_file` (mode 64). Then the ring 3 program
[file_permission_bypass.s](file_permission_bypass.s) runs as guest and
tries four syscalls:

| Try | Expected answer |
|---|---|
| open `/home/root/secret` for reading | denied: `/home/root` and the file are private |
| open `/seclab_root_file` for writing | denied: others may only read it |
| unlink `/seclab_root_file` | denied: guest does not own it |
| mkdir `/seclab_evil` | denied: `/` belongs to root |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the four tries |
| `live` | runs them; every one should say `denied` |

`seclab repair` removes the two files again.

## Try it

```
su
seclab attack file_permission_bypass live
seclab repair
```
