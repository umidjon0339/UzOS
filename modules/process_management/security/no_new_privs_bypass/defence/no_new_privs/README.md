# Defence: no_new_privs

| | |
|---|---|
| **Stops** | [`no_new_privs_bypass`](../../) |
| **Protects** | privilege: nothing a sandboxed process starts is stronger than it |
| **Called from** | `sys_seccomp()` (filter); `sys_prctl()` (only 1 accepted), `process_create()` (inherited) |

## The rule

1. A normal user installs a seccomp filter only with `no_new_privs` set
   (root: always).
2. `no_new_privs` can be turned on, never off.
3. Every child inherits it, and the seccomp sandbox with it.

## Try it

```
seclab verify no_new_privs
seclab attack no_new_privs_bypass live
```
