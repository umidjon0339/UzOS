# Defence: env_sanitize

| | |
|---|---|
| **Stops** | [`env_injection`](../../) |
| **Protects** | trust: the new program's environment |
| **Called from** | `sys_execve()`, for every envp entry |

## The rule

1. `NAME=value`, NAME of letters, digits and `_`, not starting with a digit.
2. Printable characters only.
3. Dropped: `LD_PRELOAD`, `LD_LIBRARY_PATH`, `LD_AUDIT`, `LD_DEBUG`,
   `IFS`, `BASH_ENV`, `ENV`, `PS4`.

Linux's dynamic linker ignores `LD_*` for setuid programs ("secure
exec"); sudo resets the environment. UzOS has neither setuid programs
nor a dynamic linker: every exec is a secure exec.

## Try it

```
seclab verify env_sanitize
```
