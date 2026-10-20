# Attack: calc_overflow

| | |
|---|---|
| **Stopped by** | [`safe_arith`](../../defence/safe_arith/) |
| **Breaks** | liveness: no input may halt the OS |
| **Severity** | high |

## What it does

`calc` runs inside the kernel. Some inputs make the CPU's divide
instruction fault, and a fault in the kernel halts everything:

| Input | Without the check |
|---|---|
| `-2147483648 / -1` | CPU exception 0, the OS halts |
| `5 / 0`, `7 % 0` | CPU exception 0, the OS halts |
| `2147483647 + 1` | prints `-2147483648` |
| `65536 * 65536` | prints `0` |
| `99999999999` | no 32-bit int can hold it |

A typed command must never be able to take the whole system down.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the inputs and their effect |
| `live` | runs each through the real `calc_eval()`, then `(2 + 3) * 4`, which must still give 20 |

## Uses

`calc_eval()` from commands/calc.

## Try it

```
seclab attack calc_overflow live
calc -2147483648 / -1
```
