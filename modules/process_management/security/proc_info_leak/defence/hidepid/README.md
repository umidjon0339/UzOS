# Defence: hidepid

| | |
|---|---|
| **Stops** | [`proc_info_leak`](../../) |
| **Protects** | disclosure: other users' process details |
| **Called from** | `procinfo`, `pwdx` (commands/), `/proc/<pid>/status` (filesystem_management core/procfs) |

## The rule

The details of a process (directory, open files, signals, kernel stack)
are shown to **its owner**, if it is dumpable (`prctl PR_SET_DUMPABLE`),
and to **CAP_PTRACE** (root). That it exists (pid, name, owner) stays
visible to everyone, as `ps` shows it.

Linux: the `/proc` mount option `hidepid`, and `ptrace_may_access()`.

## Try it

```
seclab verify hidepid
su guest
procinfo 2          # root's shell: details hidden
```
