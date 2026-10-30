# Attack: process_masquerade

| | |
|---|---|
| **Stopped by** | [`name_guard`](defence/name_guard/) |
| **Breaks** | trust: `ps` must show who a process really is |
| **Severity** | medium |

## What it does

People and tools find processes by name. Malware renames itself to look
like part of the OS (`kernel_main`), so nobody dares to kill it. A name
with an escape sequence repaints the screen of the admin who runs `ps`.

## In UzOS

[process_masquerade.s](process_masquerade.s) runs in ring 3 as uid 1 and
calls `prctl(PR_SET_NAME, ...)` three times: `kernel_main`, `ESC[2Jx`,
and a normal `masq_ok`. The first two must be refused, the last must work.

## Try it

```
seclab attack process_masquerade live
audit
```
