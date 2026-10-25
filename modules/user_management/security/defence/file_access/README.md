# Defence: file_access

| | |
|---|---|
| **Stops** | [`file_permission_bypass`](../../attack/file_permission_bypass/) |
| **Protects** | isolation: a user's files are theirs |
| **Called from** | `perm_check()` / `perm_may_remove()` / `perm_may_chmod()` in [core/perm](../../../core/perm/perm.c), used by `vfs_lookup`, `sys_open`, `sys_mkdir`, `sys_unlink`, `sys_rename`, `sys_chdir`, `ls`, `chmod` and the ELF loader |

## The rule

```
CAP_FILES (root)          -> allowed
the file's owner          -> the FIRST digit of the mode
everybody else            -> the SECOND digit
each digit = 4 (read) + 2 (write); the wanted bits must all be there

remove / rename / chmod   -> also: you own it (or hold CAP_FILES)
```

| Mode | Shown | Owner | Others |
|---|---|---|---|
| `64` | `rw/r-` | read, write | read |
| `60` | `rw/--` | read, write | nothing |
| `40` | `r-/--` | read | nothing |

## Why

Users without permissions have names but nothing of their own: anyone
could read, change or delete anyone's files. The check sits in the
syscalls, so every command and every program goes through it.

## Uses

- `vfs_node_t.owner` / `.mode` from filesystem_management
- `caps_has(uid, CAP_FILES)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit)

## Try it

```
ls /home                         # root/ is rw/-- : private
chmod 60 notes
seclab verify file_access
```
