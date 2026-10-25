# Defence: safe_arith

| | |
|---|---|
| **Stops** | [`calc_overflow`](../../attack/calc_overflow/) |
| **Protects** | liveness: no input can halt the OS |
| **Called from** | `calc_eval()` in [commands/calc](../../../commands/calc/calc.c), after every number and operation |

## The rules

1. every number and result fits a 32-bit `int`
2. never divide (or `%`) by 0
3. never divide `-2147483648` by `-1`

## Why

On x86, the `idiv` instruction raises **CPU exception 0** for two inputs:

| Input | Why it faults |
|---|---|
| `5 / 0` | division by zero |
| `-2147483648 / -1` | the answer, +2147483648, does not fit 32 bits |

In a user program that would kill the program. In the kernel, where the
old `calc` ran, it **halts the whole OS**. Silent overflow is the quieter
bug: `2147483647 + 1` printed `-2147483648`.

`calc` now does every step in 64 bits (where these cannot happen) and
checks the result against the 32-bit range before going on.

## Uses

- `audit_log()` from security_management (core/audit)

## Try it

```
calc -2147483648 / -1            # refused, the OS keeps running
seclab attack calc_overflow live
```
