# Attack: proc_info_leak

| | |
|---|---|
| **Stopped by** | [`hidepid`](defence/hidepid/) |
| **Breaks** | disclosure: what a user is doing is their business |
| **Severity** | medium |

## What it does

The kernel knows each process's working directory, open files and
signal masks. A tool that shows them to anyone (`procinfo`, `pwdx`,
`/proc/<pid>/status`) tells a stranger which files you are editing.

## In UzOS

| Mode | What happens |
|---|---|
| `live` | starts root's `secret_job` and user 1's `my_job`, asks hidepid as user 1 about both |
| `repair` | kills both |

Try it for real: `su guest`, then `procinfo 2` (the shell, root's) and
`pwdx 2`: the details are hidden.

## Try it

```
seclab attack proc_info_leak live
seclab repair
```
