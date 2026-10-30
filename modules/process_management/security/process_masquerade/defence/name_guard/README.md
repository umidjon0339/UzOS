# Defence: name_guard

| | |
|---|---|
| **Stops** | [`process_masquerade`](../../) |
| **Protects** | trust: a process name says who it is |
| **Called from** | `sys_prctl(PR_SET_NAME)` in [syscalls/prctl](../../../../syscalls/prctl/) |

## The rule

1. Printable characters only (0x20..0x7E): no escape sequences, no newlines.
2. Not empty.
3. A normal process may not take the name of a system process
   (`idle`, `kernel_main`, ...: whatever has `is_system` now).

## Try it

```
seclab verify name_guard
seclab attack process_masquerade live
```
